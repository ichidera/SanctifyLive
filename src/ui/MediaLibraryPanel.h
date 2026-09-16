#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

#include "core/storage/MediaLibraryStore.h"

class QEvent;
class QTreeWidget;
class QTreeWidgetItem;
class QGridLayout;
class QLabel;
class QToolButton;
class QStackedWidget;
class QScrollArea;

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

protected:
    // Watches each folder's scroll-area viewport for resizes so tiles
    // reflow to the new width -- see relayoutFolder(). A plain
    // resizeEvent() override on this panel isn't enough: the viewport
    // can change width without this panel itself resizing (e.g. a
    // scrollbar appearing/disappearing eats into it from the splitter's
    // other side).
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // One folder's worth of state: its own page (so QStackedWidget can
    // show exactly one folder's grid at a time) and its own tile list,
    // so Images and Videos reflow independently. Tiles are stored as
    // QWidget* (not the anonymous-namespace MediaTile* they actually
    // are) since that's all relayoutFolder() needs, and it keeps
    // MediaTile a pure implementation detail of the .cpp -- nothing in
    // this header has to know it exists.
    struct MediaFolder
    {
        QScrollArea *page = nullptr;
        QGridLayout *grid = nullptr;
        QVector<QWidget *> tiles;
    };

    void buildFolderTree(QTreeWidget *tree);
    MediaFolder buildImagesFolder();
    MediaFolder buildVideosFolder();
    QWidget *buildBottomBar();

    void addTile(MediaFolder &folder, const QString &name, const QColor &color,
                 const QString &imagePath, bool interactive);

    // Re-flows a folder's tiles into however many equal-width columns
    // currently fit its viewport, stretching each tile to fill the row
    // rather than leaving unclaimed space down the right edge. Called
    // whenever a tile is added and whenever the folder's viewport width
    // changes (see eventFilter()).
    void relayoutFolder(MediaFolder &folder);

    void onTreeSelectionChanged(QTreeWidgetItem *current);
    void onAddButtonClicked();
    void refreshItemCount();
    bool isVideosFolderSelected() const;

    // Restores previously-imported (real file) tiles from
    // MediaLibraryStore, called once from the constructor after the
    // built-in Images swatches exist. Entries whose file no longer
    // exists (moved/deleted since last run) are silently dropped -- see
    // MediaLibraryStore.h for why that's the right call -- and the
    // pruned list is written back so the store doesn't keep growing
    // stale rows forever.
    void loadPersistedMedia();

    // Appends one imported item to the in-memory catalog and persists
    // the whole catalog immediately. Called right after a successful
    // import in onAddButtonClicked(); kept as its own step (rather than
    // folded into addTile(), which built-in swatches also use) so
    // swatches never get written to disk as if they were real files.
    void persistImportedMedia(const QString &name, const QString &filePath, bool isVideo);

    QTreeWidget *m_tree = nullptr;
    QStackedWidget *m_stack = nullptr;
    MediaFolder m_imagesFolder;
    MediaFolder m_videosFolder;
    QLabel *m_itemCountLabel = nullptr;
    QToolButton *m_addButton = nullptr;

    // The in-memory mirror of what's on disk in MediaLibraryStore --
    // built-in swatches are NOT in here, only real imported files. Kept
    // around (rather than re-reading the file on every import) so
    // persistImportedMedia() can append-and-rewrite in one step.
    QVector<MediaLibraryStore::Entry> m_importedEntries;
};
