#pragma once

#include <QColor>
#include <QWidget>

class QTreeWidget;
class QGridLayout;

// MediaLibraryPanel is the "Media" tab: a folder tree on the left
// (Videos / Images / Feeds / DVD / Time / Audio / Collections) and a
// thumbnail grid on the right, matching the reference layout.
//
// There is no real media pipeline yet (see README roadmap: "Background
// media (image/video) per slide" is still unbuilt), so this panel ships
// a small built-in set of solid-color swatches under "Images" rather
// than pretending to browse a real file library. What IS real: double-
// clicking a swatch asks the operator window to add a new slide using
// that color as its background, via mediaActivated() -- a genuine,
// if modest, bridge from this panel into ScheduleModel rather than a
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

private:
    void buildFolderTree(QTreeWidget *tree);
    void buildThumbnailGrid(QGridLayout *grid);

    QGridLayout *m_grid = nullptr;
};
