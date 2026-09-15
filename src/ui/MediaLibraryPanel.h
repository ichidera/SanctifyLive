#pragma once

#include <QColor>
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class QGridLayout;
class QLabel;
class QToolButton;
class QStackedWidget;

// MediaLibraryPanel is the MEDIA tab (one of the Content Tabs -- see
// OperatorWindow's panel glossary): a folder tree on the left (Videos,
// Images), a thumbnail grid for whichever folder is selected, and a
// bottom bar (add / settings / item count / view) matching the
// reference layout. This panel does NOT render its own preview -- the
// "how would this actually look" rendering lives one level up, in
// OperatorWindow's Item Preview panel (next to History), so that a
// single preview surface can eventually serve every Content Tab, not
// just Media. This panel's only job is to tell OperatorWindow *what*
// to preview, via previewRequested().
//
// Videos and Images are genuinely separate collections (own grid, own
// item count, own "+" import filter) -- not the same tree-item-as-
// decoration trick used elsewhere. That's deliberate: video is going to
// need real decode/thumbnail/playback machinery (VLC/FFmpeg-level work)
// that has nothing to do with how images work, so this panel keeps them
// structurally apart from day one instead of papering over the
// difference with a shared grid.
//
// There is no real media pipeline yet (see README roadmap: "Background
// media (image/video) per slide" is still unbuilt -- Slide only has a
// solid QColor background, no image/video path), so Images ships a
// small built-in set of solid-color swatches rather than pretending to
// browse a real file library, and Videos starts empty. What IS real:
//   - A single click on an interactive tile fires previewRequested() so
//     OperatorWindow can render it in the Item Preview panel;
//     double-clicking asks the operator window to add a new slide using
//     that color as its background, via mediaActivated().
//   - The bottom bar's "+" button genuinely opens a file picker -- filtered
//     to real image extensions when Images is selected, real video
//     extensions when Videos is selected, straight to the picker, no
//     "what kind of file?" menu in between since the current folder
//     already answers that.
//   - Imported images get an actual thumbnail of the real file (not a
//     placeholder swatch) -- browsing is real. Because Slide can't carry
//     an image background yet, activating/previewing an imported image
//     tile still falls back to a neutral placeholder color rather than
//     pretending the real photo would appear live; its tooltip says so.
//   - Imported videos are cataloged (real file, real name) but not
//     interactive yet -- no thumbnail (no decoder wired in), and
//     clicking/double-clicking does nothing, since there's no honest
//     "this is what it'll look like live" answer to give for video yet.
//   - The item count in the bottom bar is read live off whichever
//     folder's grid is currently showing, never hardcoded.
class MediaLibraryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MediaLibraryPanel(QWidget *parent = nullptr);

signals:
    // Emitted when the operator double-clicks an interactive media tile.
    // `name` is shown to the user (e.g. as the new slide's label);
    // `color` is the swatch color to use as that slide's background.
    void mediaActivated(const QString &name, const QColor &color);

    // Emitted on a single click/press of an interactive tile, before any
    // double-click has a chance to register. `name`/`color` describe the
    // same swatch as above; OperatorWindow uses this to update its Item
    // Preview panel without anything being added to the Schedule -- the
    // Media-tab equivalent of how selecting (not activating) a Schedule
    // row only updates Preview.
    void previewRequested(const QString &name, const QColor &color);

private:
    // One folder's worth of state: its own page (so QStackedWidget can
    // show exactly one folder's grid at a time) and its own grid
    // placement cursor (so Images and Videos fill their grids
    // independently instead of sharing row/col counters).
    struct MediaFolder
    {
        QWidget *page = nullptr;
        QGridLayout *grid = nullptr;
        int nextRow = 0;
        int nextCol = 0;
    };

    void buildFolderTree(QTreeWidget *tree);
    MediaFolder buildImagesFolder();
    MediaFolder buildVideosFolder();
    QWidget *buildBottomBar();

    void addTile(MediaFolder &folder, const QString &name, const QColor &color,
                 const QString &thumbnailPath, bool interactive);
    void onTreeSelectionChanged(QTreeWidgetItem *current);
    void onAddButtonClicked();
    void refreshItemCount();
    bool isVideosFolderSelected() const;

    QTreeWidget *m_tree = nullptr;
    QStackedWidget *m_stack = nullptr;
    MediaFolder m_imagesFolder;
    MediaFolder m_videosFolder;
    QLabel *m_itemCountLabel = nullptr;
    QToolButton *m_addButton = nullptr;
};
