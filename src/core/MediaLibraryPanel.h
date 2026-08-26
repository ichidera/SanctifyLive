#pragma once

#include <QWidget>

class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;

// The bottom resource library: a row of content-type tabs (Songs,
// Scriptures, Media, Presentations, Themes), and -- for the Media tab,
// the only one built out so far -- a category tree on the left, a
// thumbnail grid in the middle, and a larger preview on the right.
//
// Only "Media" is functional right now; the others are visible-but-
// disabled placeholders so the tab row doesn't need reshuffling once
// they're built (see build-plan steps 4 & 6). Everything inside is
// placeholder sample data (flat color swatches standing in for real
// photos) purely to prove the layout -- swapping in a real asset/import
// pipeline later only touches this file.
//
// All internal splits use QSplitter, so the tree/grid/preview widths
// are user-resizable, matching the rest of the app's panels.
class MediaLibraryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MediaLibraryPanel(QWidget *parent = nullptr);

signals:
    // Emitted when the user double-clicks (or otherwise activates) a
    // media item to send it into the schedule as a new slide.
    void mediaActivated(const QString &label, const QColor &background);

private slots:
    void onCategorySelected(QTreeWidgetItem *item, int column);
    void onGridSelectionChanged();
    void onGridItemActivated(QListWidgetItem *item);

private:
    void populateSampleMedia();
    void rebuildGridForCategory(const QString &category);

    QTabWidget *m_contentTabs;
    QTreeWidget *m_categoryTree;
    QListWidget *m_mediaGrid;
    QLabel *m_previewImage;
    QLabel *m_previewCaption;
    QLabel *m_itemCountLabel;
    QLineEdit *m_searchBox;

    // category name -> list of (label, color) pairs
    QMap<QString, QVector<QPair<QString, QColor>>> m_sampleData;
};