#pragma once

#include <QColor>
#include <QWidget>

class QTreeWidget;
class QGridLayout;
class QLabel;
class SlideCanvas;

// MediaLibraryPanel is the "Media" tab: a folder tree on the left
// (currently just Videos and Images), an item-preview pane in the
// middle showing exactly how the currently selected item would look if
// it were put on air, and a thumbnail grid on the right, matching the
// reference layout. The middle preview pane is deliberately its own
// panel (not a tooltip or a bigger thumbnail) so the operator can see,
// at a glance, how a background will actually render before adding it.
//
// There is no real media pipeline yet (see README roadmap: "Background
// media (image/video) per slide" is still unbuilt), so this panel ships
// a small built-in set of solid-color swatches under "Images" rather
// than pretending to browse a real file library. What IS real: a single
// click on a tile updates the preview pane (using the exact same
// SlideRenderer/SlideCanvas pipeline the Schedule/Preview/Live panes
// use, so what you see here is pixel-faithful to what you'd get);
// double-clicking asks the operator window to add a new slide using
// that color as its background, via mediaActivated() -- a genuine, if
// modest, bridge from this panel into ScheduleModel rather than a
// purely decorative mock.
class MediaLibraryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MediaLibraryPanel(QWidget *parent = nullptr);

signals:
    // Emitted when the operator double-clicks a media tile. `name` is
    // shown to the user (e.g. as the new slide's label); `color` is the
    // swatch color to use as that slide's background.
    void mediaActivated(const QString &name, const QColor &color);

private slots:
    void onTilePreviewRequested(const QString &name, const QColor &color);

private:
    void buildFolderTree(QTreeWidget *tree);
    void buildThumbnailGrid(QGridLayout *grid);

    QGridLayout *m_grid = nullptr;
    SlideCanvas *m_previewCanvas = nullptr;
    QLabel *m_previewLabel = nullptr;
};
