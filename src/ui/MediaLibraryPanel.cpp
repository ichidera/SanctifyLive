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

protected:
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

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter);
}

void MediaLibraryPanel::buildFolderTree(QTreeWidget *tree)
{
    const QStringList folders = {
        tr("Videos"), tr("Images"), tr("Feeds"), tr("DVD"),
        tr("Time"), tr("Audio"), tr("Collections"),
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
        grid->addWidget(tile, row, col);
        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }
}

#include "MediaLibraryPanel.moc"
