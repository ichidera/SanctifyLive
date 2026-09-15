#include "ui/MediaLibraryPanel.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{

// Extensions worth offering today. Images actually render (as a
// fallback swatch color -- see MediaTile below); video is catalog-only
// for now, no decoder wired in, but real VLC/FFmpeg-level work is
// planned (see README roadmap), so the filter already reflects the
// common containers rather than a token single extension.
const char *kSupportedImageFilter =
    "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.svg)";
const char *kSupportedVideoFilter =
    "Videos (*.mp4 *.mov *.mkv *.avi *.webm)";

// A single thumbnail tile. Kept local to this .cpp (rather than its own
// header) since MediaLibraryPanel is its only user. It needs Q_OBJECT
// for the `activated` signal, which is why this file ends with an
// explicit `#include "MediaLibraryPanel.moc"` -- classes with Q_OBJECT
// defined inside a .cpp (rather than a header) need that so CMake's
// AUTOMOC can generate and compile their moc code.
class MediaTile : public QFrame
{
    Q_OBJECT

public:
    // Built-in solid-color swatch (the original six Images placeholders).
    MediaTile(QString name, QColor color, QWidget *parent = nullptr)
        : MediaTile(std::move(name), color, QPixmap(), /*interactive=*/true, parent)
    {
    }

    // `thumbnail`, if non-null, is an actual downscaled render of a real
    // imported file, so browsing is genuine rather than a placeholder.
    // `color` is a fallback used only if/when this tile is activated or
    // previewed -- Slide has no image-background field yet (see README
    // roadmap), so we can't honestly show the real picture live.
    //
    // `interactive` controls whether clicking/double-clicking does
    // anything at all. Imported video tiles pass false: there's no
    // honest "this is what it'll look like live" answer to give for
    // video yet (no decoder, no renderer support), so rather than fake
    // an add-to-schedule action that doesn't mean what it looks like it
    // means, video tiles are catalog-only until that's real.
    MediaTile(QString name, QColor color, QPixmap thumbnail, bool interactive, QWidget *parent = nullptr)
        : QFrame(parent), m_name(std::move(name)), m_color(color), m_interactive(interactive)
    {
        setObjectName("mediaTile");
        setCursor(interactive ? Qt::PointingHandCursor : Qt::ArrowCursor);
        setFixedSize(104, 96);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);

        if (thumbnail.isNull()) {
            auto *swatch = new QFrame(this);
            swatch->setFixedHeight(56);
            swatch->setStyleSheet(
                QString("background-color: %1; border-radius: 5px; border: none;")
                    .arg(m_color.name()));
            layout->addWidget(swatch);
        } else {
            auto *thumb = new QLabel(this);
            thumb->setFixedHeight(56);
            thumb->setAlignment(Qt::AlignCenter);
            thumb->setScaledContents(false);
            thumb->setPixmap(thumbnail.scaled(92, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            thumb->setStyleSheet("border-radius: 5px;");
            layout->addWidget(thumb, 0, Qt::AlignCenter);
        }

        if (!interactive) {
            setToolTip(tr("Video is cataloged here, but playback and adding it to the "
                          "Schedule aren't implemented yet -- see the roadmap in README.md."));
        } else if (!thumbnail.isNull()) {
            setToolTip(tr("Imported image -- browsing shows the real file, but putting it "
                          "live still uses a placeholder color (image backgrounds aren't "
                          "wired into the renderer yet; see README roadmap)."));
        }

        auto *label = new QLabel(m_name, this);
        label->setObjectName("mediaTileLabel");
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        layout->addWidget(label);
    }

signals:
    void activated(const QString &name, const QColor &color);
    // Emitted on a single click/press so MediaLibraryPanel can forward
    // it up to OperatorWindow's Item Preview panel, without committing
    // anything to the schedule -- mirrors how clicking a Schedule row
    // stages Preview without going live.
    void previewRequested(const QString &name, const QColor &color);

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        QFrame::mousePressEvent(event);
        if (m_interactive)
            emit previewRequested(m_name, m_color);
    }

    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        QFrame::mouseDoubleClickEvent(event);
        if (m_interactive)
            emit activated(m_name, m_color);
    }

private:
    QString m_name;
    QColor m_color;
    bool m_interactive;
};

} // namespace

MediaLibraryPanel::MediaLibraryPanel(QWidget *parent) : QWidget(parent)
{
    auto *splitter = new QSplitter(this);

    m_tree = new QTreeWidget(splitter);
    m_tree->setHeaderHidden(true);
    m_tree->setMaximumWidth(200);
    buildFolderTree(m_tree);
    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        onTreeSelectionChanged(current);
    });
    splitter->addWidget(m_tree);

    m_stack = new QStackedWidget(splitter);
    m_videosFolder = buildVideosFolder();
    m_imagesFolder = buildImagesFolder();
    m_stack->addWidget(m_videosFolder.page);
    m_stack->addWidget(m_imagesFolder.page);
    m_stack->setCurrentWidget(m_imagesFolder.page); // matches buildFolderTree()'s default selection
    splitter->addWidget(m_stack);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(splitter, /*stretch=*/1);
    layout->addWidget(buildBottomBar());
}

void MediaLibraryPanel::buildFolderTree(QTreeWidget *tree)
{
    // Feeds/DVD/Time/Audio/Collections removed for now -- there's no
    // backing feature for any of them yet (no live feed ingestion, no
    // disc playback, no countdown timers, no audio playback, no saved
    // collections), so listing them would just be decoration pretending
    // to be functionality. Videos and Images are real, separate
    // collections (see this class's own doc comment for why they're
    // kept structurally apart rather than sharing one grid).
    auto *videosItem = new QTreeWidgetItem(tree, {tr("Videos")});
    auto *imagesItem = new QTreeWidgetItem(tree, {tr("Images")});
    Q_UNUSED(videosItem);
    tree->setCurrentItem(imagesItem);
}

MediaLibraryPanel::MediaFolder MediaLibraryPanel::buildImagesFolder()
{
    MediaFolder folder;
    auto *scrollArea = new QScrollArea(m_stack);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *gridHost = new QWidget(scrollArea);
    folder.grid = new QGridLayout(gridHost);
    folder.grid->setSpacing(12);
    folder.grid->setContentsMargins(12, 12, 12, 12);
    folder.grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scrollArea->setWidget(gridHost);
    folder.page = scrollArea;

    // A small built-in set of solid-color swatches standing in for a
    // real image library (see README roadmap).
    struct Swatch { const char *name; const char *hex; };
    static const Swatch swatches[] = {
        {"Beach Sunset", "#e08a3c"},
        {"Blue Paint", "#2f6fed"},
        {"Cross Sunset", "#7a2e2e"},
        {"Tree", "#3f6b3a"},
        {"Yellow Sky", "#d4a72c"},
        {"Sun and Clouds", "#b8c9d6"},
    };
    for (const Swatch &swatch : swatches)
        addTile(folder, tr(swatch.name), QColor(swatch.hex), QString(), /*interactive=*/true);

    return folder;
}

MediaLibraryPanel::MediaFolder MediaLibraryPanel::buildVideosFolder()
{
    // Starts empty -- no built-in placeholders, since a fake solid-color
    // "video" swatch would be actively misleading about what video
    // support currently is (nothing yet; see README roadmap).
    MediaFolder folder;
    auto *scrollArea = new QScrollArea(m_stack);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *gridHost = new QWidget(scrollArea);
    folder.grid = new QGridLayout(gridHost);
    folder.grid->setSpacing(12);
    folder.grid->setContentsMargins(12, 12, 12, 12);
    folder.grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scrollArea->setWidget(gridHost);
    folder.page = scrollArea;
    return folder;
}

QWidget *MediaLibraryPanel::buildBottomBar()
{
    // Mirrors the reference status bar: "+" and a settings button on the
    // left, the live item count centered, a view-options button on the
    // right. All four widgets are real Qt controls -- Settings and View
    // are honest disabled stubs (no backing feature yet, same pattern as
    // Transcription/Profiles elsewhere in this app) rather than buttons
    // that look clickable but silently do nothing.
    m_addButton = new QToolButton(this);
    m_addButton->setText(tr("+"));
    m_addButton->setAutoRaise(true);
    connect(m_addButton, &QToolButton::clicked, this, &MediaLibraryPanel::onAddButtonClicked);

    auto *settingsButton = new QToolButton(this);
    settingsButton->setText(tr("\u2699"));
    settingsButton->setAutoRaise(true);
    settingsButton->setEnabled(false);
    settingsButton->setToolTip(tr("Media settings aren't implemented yet -- see the roadmap in README.md."));

    auto *viewButton = new QToolButton(this);
    viewButton->setText(tr("\u25A6"));
    viewButton->setAutoRaise(true);
    viewButton->setEnabled(false);
    viewButton->setToolTip(tr("View options aren't implemented yet -- see the roadmap in README.md."));

    m_itemCountLabel = new QLabel(this);
    m_itemCountLabel->setObjectName("nextSlideLabel");
    m_itemCountLabel->setAlignment(Qt::AlignCenter);

    auto *bar = new QWidget(this);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->addWidget(m_addButton);
    layout->addWidget(settingsButton);
    layout->addStretch(1);
    layout->addWidget(m_itemCountLabel);
    layout->addStretch(1);
    layout->addWidget(viewButton);

    refreshItemCount();
    return bar;
}

bool MediaLibraryPanel::isVideosFolderSelected() const
{
    return m_tree->currentItem() && m_tree->currentItem()->text(0) == tr("Videos");
}

void MediaLibraryPanel::onTreeSelectionChanged(QTreeWidgetItem *current)
{
    if (!current)
        return;
    m_stack->setCurrentWidget(current->text(0) == tr("Videos") ? m_videosFolder.page : m_imagesFolder.page);
    m_addButton->setToolTip(current->text(0) == tr("Videos") ? tr("Add a video file") : tr("Add an image file"));
    refreshItemCount();
}

void MediaLibraryPanel::onAddButtonClicked()
{
    // No "what kind of file?" menu -- the currently-selected folder
    // already answers that (see this class's own doc comment for why
    // Videos and Images are kept as separate collections rather than
    // one grid with a type picker on import).
    if (isVideosFolderSelected()) {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Add Video"), QString(), tr(kSupportedVideoFilter),
            nullptr, QFileDialog::DontUseNativeDialog);
        if (path.isEmpty())
            return;
        // No thumbnail: no decoder wired in yet (see README roadmap).
        // Not interactive: see MediaTile's doc comment on why a video
        // tile shouldn't pretend clicking it means something yet.
        addTile(m_videosFolder, QFileInfo(path).completeBaseName(), QColor("#3a3f4b"), QString(), /*interactive=*/false);
        return;
    }

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Add Image"), QString(), tr(kSupportedImageFilter),
        nullptr, QFileDialog::DontUseNativeDialog);
    if (path.isEmpty())
        return;

    QPixmap pixmap(path);
    if (pixmap.isNull())
        return; // not a decodable image -- fail quietly rather than adding a broken tile

    // A cheap, honest stand-in for "the image's color" until Slide can
    // carry a real background image (see README roadmap): downscaling
    // to a single pixel with smooth interpolation gives an average of
    // the whole picture, not just one corner.
    const QColor averageColor = pixmap.scaled(1, 1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation).toImage().pixelColor(0, 0);

    addTile(m_imagesFolder, QFileInfo(path).completeBaseName(), averageColor, path, /*interactive=*/true);
}

void MediaLibraryPanel::addTile(MediaFolder &folder, const QString &name, const QColor &color,
                                 const QString &thumbnailPath, bool interactive)
{
    constexpr int columns = 3;

    MediaTile *tile = thumbnailPath.isEmpty()
                           ? new MediaTile(name, color, QPixmap(), interactive)
                           : new MediaTile(name, color, QPixmap(thumbnailPath), interactive);
    connect(tile, &MediaTile::activated, this, &MediaLibraryPanel::mediaActivated);
    connect(tile, &MediaTile::previewRequested, this, &MediaLibraryPanel::previewRequested);
    folder.grid->addWidget(tile, folder.nextRow, folder.nextCol);

    if (++folder.nextCol >= columns) {
        folder.nextCol = 0;
        ++folder.nextRow;
    }

    refreshItemCount();
}

void MediaLibraryPanel::refreshItemCount()
{
    // Called from addTile(), which buildImagesFolder() calls while the
    // constructor is still assembling the tree+grid splitter -- before
    // buildBottomBar() (and therefore m_itemCountLabel) exists. Guard
    // rather than reorder construction, since buildBottomBar() also
    // calls this once at the end to set the initial count correctly.
    if (!m_itemCountLabel)
        return;

    // Read live off whichever folder's grid is currently showing, never
    // tracked separately, so this can never drift out of sync with
    // what's actually on screen -- and so switching folders updates the
    // count to match without any extra bookkeeping.
    const QGridLayout *currentGrid = isVideosFolderSelected() ? m_videosFolder.grid : m_imagesFolder.grid;
    const int count = currentGrid ? currentGrid->count() : 0;
    m_itemCountLabel->setText(tr("%n item(s)", "", count));
}

#include "MediaLibraryPanel.moc"
