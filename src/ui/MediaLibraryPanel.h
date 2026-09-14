#pragma once

#include <QColor>
#include <QWidget>

class QTreeWidget;
class QGridLayout;

// MediaLibraryPanel is the MEDIA tab (one of the Content Tabs -- see
// OperatorWindow's panel glossary): a folder tree on the left (currently
// just Videos and Images) and a thumbnail grid on the right, matching
// the reference layout. This panel does NOT render its own preview --
// the "how would this actually look" rendering lives one level up, in
// OperatorWindow's Item Preview panel (next to History), so that a
// single preview surface can eventually serve every Content Tab, not
// just Media. This panel's only job is to tell OperatorWindow *what*
// to preview, via previewRequested().
//
// There is no real media pipeline yet (see README roadmap: "Background
// media (image/video) per slide" is still unbuilt), so this panel ships
// a small built-in set of solid-color swatches under "Images" rather
// than pretending to browse a real file library. What IS real: a single
// click on a tile fires previewRequested() so OperatorWindow can render
// it in the Item Preview panel; double-clicking asks the operator
// window to add a new slide using that color as its background, via
// mediaActivated() -- a genuine, if modest, bridge from this panel into
// ScheduleModel rather than a purely decorative mock.
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

    // Emitted on a single click/press, before any double-click has a
    // chance to register. `name`/`color` describe the same swatch as
    // above; OperatorWindow uses this to update its Item Preview panel
    // without anything being added to the Schedule -- the Media-tab
    // equivalent of how selecting (not activating) a Schedule row only
    // updates Preview.
    void previewRequested(const QString &name, const QColor &color);

private:
    void buildFolderTree(QTreeWidget *tree);
    void buildThumbnailGrid(QGridLayout *grid);

    QGridLayout *m_grid = nullptr;
};
