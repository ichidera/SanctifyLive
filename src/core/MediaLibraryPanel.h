#ifndef SANCTIFYLIVE_CORE_MEDIALIBRARYPANEL_H_
#define SANCTIFYLIVE_CORE_MEDIALIBRARYPANEL_H_


#include <QColor>
#include <QPointF>
#include <QWidget>

class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;
class QToolButton;
class ScripturePanel;

// The bottom resource library: a row of content-type tabs (Songs,
// Scriptures, Media, Presentations, Themes). "Media" has a category
// tree on the left, a thumbnail grid in the middle, and a larger
// preview on the right; "Scriptures" hosts a ScripturePanel (see
// src/core/scripture/ScripturePanel.h) with its own book/chapter/verse
// browser and search box.
//
// "Media" and "Scriptures" are functional; Songs/Presentations/Themes
// are still visible-but-disabled placeholders. A handful of sample
// color-swatch entries ship built in purely to prove the layout;
// anything the user imports via the "+" button is real (a real image
// file, or a placeholder for non-image types until those get proper
// thumbnailing/playback).
//
// All internal splits use QSplitter, so the tree/grid/preview widths
// are user-resizable, matching the rest of the app's panels.
class MediaLibraryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MediaLibraryPanel(QWidget *parent = nullptr);

signals:
    // Emitted when the user activates (double-clicks, or "Go Live" from
    // the right-click menu) a media item to send it live everywhere.
    // imagePath is empty for the placeholder color swatches; when
    // non-empty, the real image should be used as the background with no
    // text overlay, cropped toward focus when a device's aspect ratio
    // doesn't match the image's (see Slide::backgroundFocus, and "Edit
    // Framing..." below).
    void mediaActivated(const QString &label, const QColor &background, const QString &imagePath,
                         const QPointF &focus);

    // Emitted from the right-click "Send to Phone Only" action: same
    // payload as mediaActivated, but meant for ScheduleModel's phone
    // override rather than the main live output -- see PROTOCOL.md's
    // "Roles" section for what that actually does.
    void mediaSentToPhone(const QString &label, const QColor &background, const QString &imagePath,
                          const QPointF &focus);

    // Forwarded from the Scriptures tab's ScripturePanel; see
    // ScripturePanel::scriptureActivated for what each argument means.
    void scriptureActivated(const QString &reference, const QString &text, const QString &translationCode);

private slots:
    void onCategorySelected(QTreeWidgetItem *item, int column);
    void onGridSelectionChanged();
    void onGridItemActivated(QListWidgetItem *item);
    void onGridContextMenu(const QPoint &pos);
    void onImportClicked();

private:
    struct MediaEntry
    {
        QString label;
        QColor color;
        QString imagePath; // empty for placeholder swatches
        // Normalized (0..1) point in imagePath that should stay in frame
        // when a cover-fit crop has to discard part of the image --
        // meaningless (and unused) for placeholder swatches. Set via the
        // right-click "Edit Framing..." action.
        QPointF focus = QPointF(0.5, 0.5);
    };

    void populateSampleMedia();
    void rebuildGridForCategory(const QString &category);
    QIcon iconForEntry(const MediaEntry &entry) const;

    QTabWidget *m_contentTabs;
    ScripturePanel *m_scripturePanel;
    QTreeWidget *m_categoryTree;
    QListWidget *m_mediaGrid;
    QLabel *m_previewImage;
    QLabel *m_previewCaption;
    QLabel *m_itemCountLabel;
    QLineEdit *m_searchBox;
    QToolButton *m_importButton;

    QString m_currentCategory;
    QMap<QString, QVector<MediaEntry>> m_sampleData;
};

#endif // SANCTIFYLIVE_CORE_MEDIALIBRARYPANEL_H_
