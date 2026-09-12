#include "ui/MediaLibraryPanel.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "core/model/Slide.h"
#include "core/render/RenderResolution.h"
#include "core/render/SlideRenderer.h"
#include "ui/SlideCanvas.h"

namespace
{

// A single clickable/double-clickable thumbnail tile. Kept local to
// this .cpp (rather than its own header) since MediaLibraryPanel is its
// only user. It needs Q_OBJECT for the `activated` signal, which is why
// this file ends with an explicit `#include "MediaLibraryPanel.moc"` --
// classes with Q_OBJECT defined inside a .cpp (rather than a header)
// need that so CMake's AUTOMOC can generate and compile their moc code.
class MediaTile : public QFrame
{
    Q_OBJECT

public:
    MediaTile(QString name, QColor color, QWidget *parent = nullptr)
        : QFrame(parent), m_name(std::move(name)), m_color(color)
    {
        setObjectName("mediaTile");
        setCursor(Qt::PointingHandCursor);
        setFixedSize(104, 96);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);

        auto *swatch = new QFrame(this);
        swatch->setFixedHeight(56);
        swatch->setStyleSheet(
            QString("background-color: %1; border-radius: 5px; border: none;")
                .arg(m_color.name()));
        layout->addWidget(swatch);

        auto *label = new QLabel(m_name, this);
        label->setObjectName("mediaTileLabel");
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        layout->addWidget(label);
    }

signals:
    void activated(const QString &name, const QColor &color);
    // Emitted on a single click/press so the preview pane can update
    // without committing anything to the schedule -- mirrors how
    // clicking a Schedule row stages Preview without going live.
    void previewRequested(const QString &name, const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        QFrame::mousePressEvent(event);
        emit previewRequested(m_name, m_color);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        QFrame::mouseDoubleClickEvent(event);
        emit activated(m_name, m_color);
    }

private:
    QString m_name;
    QColor m_color;
};

} // namespace

MediaLibraryPanel::MediaLibraryPanel(QWidget *parent) : QWidget(parent)
{
    auto *splitter = new QSplitter(this);

    auto *tree = new QTreeWidget(splitter);
    tree->setHeaderHidden(true);
    tree->setMaximumWidth(200);
    buildFolderTree(tree);
    splitter->addWidget(tree);

    // Item-preview pane: deliberately sits between the folder tree and
    // the thumbnail grid, not off to one side, so it reads as "select on
    // the left, see it big in the middle, browse more on the right."
    // Shares the exact rendering pipeline (SlideRenderer + kDesignResolution
    // + SlideCanvas) that the Schedule/Preview/Live panes use, so this is
    // a pixel-faithful preview of how the item will actually appear once
    // it's put on air -- not a separate, possibly-inconsistent mockup.
    auto *previewWrapper = new QWidget(splitter);
    auto *previewLayout = new QVBoxLayout(previewWrapper);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);

    m_previewCanvas = new SlideCanvas(previewWrapper);
    m_previewCanvas->setMinimumWidth(200);

    m_previewLabel = new QLabel(tr("Select an item to preview"), previewWrapper);
    m_previewLabel->setObjectName("nextSlideLabel");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setWordWrap(true);

    previewLayout->addWidget(m_previewCanvas, /*stretch=*/1);
    previewLayout->addWidget(m_previewLabel);
    splitter->addWidget(previewWrapper);

    onTilePreviewRequested(QString(), QColor()); // show an empty canvas until something is picked

    auto *scrollArea = new QScrollArea(splitter);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *gridHost = new QWidget(scrollArea);
    m_grid = new QGridLayout(gridHost);
    m_grid->setSpacing(12);
    m_grid->setContentsMargins(12, 12, 12, 12);
    m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    buildThumbnailGrid(m_grid);
    scrollArea->setWidget(gridHost);
    splitter->addWidget(scrollArea);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);
}

void MediaLibraryPanel::onTilePreviewRequested(const QString &name, const QColor &color)
{
    if (!color.isValid()) {
        m_previewCanvas->setPixmap(SlideRenderer::render(nullptr, kDesignResolution));
        m_previewLabel->setText(tr("Select an item to preview"));
        return;
    }

    // Preview with an empty text body, matching exactly what
    // onMediaActivated() actually adds to the schedule (a slide with
    // this color as its background and no text) -- so what's shown here
    // never diverges from what double-clicking would produce.
    const Slide previewSlide(name, QString(), color);
    m_previewCanvas->setPixmap(SlideRenderer::render(&previewSlide, kDesignResolution));
    m_previewLabel->setText(name);
}

void MediaLibraryPanel::buildFolderTree(QTreeWidget *tree)
{
    // Feeds/DVD/Time/Audio/Collections removed for now -- there's no
    // backing feature for any of them yet (no live feed ingestion, no
    // disc playback, no countdown timers, no audio playback, no saved
    // collections), so listing them would just be decoration pretending
    // to be functionality. Videos stays as a visible placeholder folder
    // (no video import yet either, but it's the very next roadmap item
    // media-wise) and Images is the one folder with real content.
    const QStringList folders = {
        tr("Videos"), tr("Images"),
    };
    QTreeWidgetItem *imagesItem = nullptr;
    for (const QString &folder : folders) {
        auto *item = new QTreeWidgetItem(tree, {folder});
        if (folder == tr("Images"))
            imagesItem = item;
    }
    if (imagesItem)
        tree->setCurrentItem(imagesItem);
}

void MediaLibraryPanel::buildThumbnailGrid(QGridLayout *grid)
{
    struct Swatch { const char *name; const char *hex; };
    static const Swatch swatches[] = {
        {"Beach Sunset", "#e08a3c"},
        {"Blue Paint", "#2f6fed"},
        {"Cross Sunset", "#7a2e2e"},
        {"Tree", "#3f6b3a"},
        {"Yellow Sky", "#d4a72c"},
        {"Sun and Clouds", "#b8c9d6"},
    };

    constexpr int columns = 3;
    int row = 0, col = 0;
    for (const Swatch &swatch : swatches) {
        auto *tile = new MediaTile(tr(swatch.name), QColor(swatch.hex));
        connect(tile, &MediaTile::activated, this, &MediaLibraryPanel::mediaActivated);
        connect(tile, &MediaTile::previewRequested, this, &MediaLibraryPanel::onTilePreviewRequested);
        grid->addWidget(tile, row, col);
        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }
}

#include "MediaLibraryPanel.moc"
