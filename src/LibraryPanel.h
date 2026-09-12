#pragma once
//
// LibraryPanel.h
// Bottom-left area: the Songs/Scriptures/Media/Presentations/Themes tab bar,
// the folder tree (Videos/Images/Feeds/...), and the media thumbnail grid.
//
#include <QWidget>

class QTreeWidget;
class QListWidget;
class QTabWidget;

class LibraryPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryPanel(QWidget* parent = nullptr);

private:
    QTabWidget* m_tabs = nullptr;
    QTreeWidget* m_folderTree = nullptr;
    QListWidget* m_thumbGrid = nullptr;

    QWidget* buildMediaTab();
    void populateFolderTree();
    void populateThumbnails();
};
