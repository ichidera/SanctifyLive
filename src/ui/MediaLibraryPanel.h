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
// Images ships a small built-in set of solid-color swatches (no
// backing file, no real library) alongside genuinely imported photos,
// and Videos starts empty. What IS real:
//   - A single click on an interactive tile fires previewRequested() so
//     OperatorWindow can render it in the Item Preview panel;
//     double-clicking asks the operator window to add a new slide using
//     it as its background, via mediaActivated().
//   - The bottom bar's "+" button goes straight to a file picker -- filtered
//     to real image extensions when Images is selected, real video
//     extensions when Videos is selected, no "what kind of file?" menu
//     in between since the current folder already answers that.
//   - Imported images carry their real file path end to end: the
//     thumbnail shown while browsing, the Item Preview render, and (if
//     activated) the live Slide's background are all the same actual
//     photo via Slide::backgroundImagePath, not a color approximation.
//     The built-in swatches have no file behind them, so they still use
//     a plain QColor background -- same Slide, same SlideRenderer path,
//     just no image path set.
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
    // `color` is the fallback/behind-image background color; `imagePath`
    // is the real imported file's path, or empty for a built-in swatch
    // (in which case `color` is the actual background, not a fallback).
    void mediaActivated(const QString &name, const QColor &color, const QString &imagePath);

    // Emitted on a single click/press of an interactive tile, before any
    // double-click has a chance to register. Same parameters as
    // mediaActivated(); OperatorWindow uses this to update its Item
    // Preview panel without anything being added to the Schedule -- the
    // Media-tab equivalent of how selecting (not activating) a Schedule
    // row only updates Preview.
    void previewRequested(const QString &name, const QColor &color, const QString &imagePath);

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
                 const QString &imagePath, bool interactive);
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
