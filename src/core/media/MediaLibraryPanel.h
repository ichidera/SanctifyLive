#ifndef SANCTIFYLIVE_CORE_MEDIALIBRARYPANEL_H_
#define SANCTIFYLIVE_CORE_MEDIALIBRARYPANEL_H_


#include <QColor>
#include <QPointF>
#include <QWidget>

#include "../settings/OutputProfile.h"

class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QLineEdit;
class QToolButton;
class ScripturePanel;
class SongPanel;
class ThemePanel;
class LiveAppearancePreview;

// The bottom resource library: a row of content-type tabs (Songs,
// Scriptures, Media, Presentations, Themes). "Media" has a category
// tree on the left, a thumbnail grid in the middle, and a larger
// preview on the right; "Scriptures" hosts a ScripturePanel (see
// src/core/scripture/ScripturePanel.h) with its own book/chapter/verse
// browser and search box; "Songs" and "Themes" host SongPanel and
// ThemePanel respectively (src/core/song/SongPanel.h,
// src/core/theme/ThemePanel.h).
//
// Media, Scriptures, Songs, and Themes are all functional; Presentations
// remains a visible-but-disabled placeholder (there's no slide-deck
// import/rendering pipeline yet). A handful of sample entries ship built
// in purely to prove each tab's layout; anything the user imports/adds
// via that tab's own "+" control is real.
//
// Every functional tab's preview -- Media's own, plus the ones owned by
// ScripturePanel/SongPanel/ThemePanel -- is the same LiveAppearancePreview
// widget (src/core/output/LiveAppearancePreview.h), fed whichever
// OutputProfile is current via setOutputProfile() below. That's what
// makes "how will this look on screen" consistent across every tab
// rather than each one inventing its own preview logic.
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

    // Forwarded from the Songs tab's SongPanel; see
    // SongPanel::songSlideActivated for what each argument means.
    void songSlideActivated(const QString &songTitle, const QString &slideText);

    // Forwarded from the Themes tab's ThemePanel; same payload shape as
    // mediaActivated above, so a theme can be sent live through exactly
    // the same path a Media entry uses.
    void themeActivated(const QString &label, const QColor &background, const QString &imagePath,
                         const QPointF &focus);

public slots:
    // Which destination's resolution/margins/font every preview in this
    // panel (Media's own, plus Scriptures/Songs/Themes) should represent.
    // See LiveAppearancePreview::setProfile.
    void setOutputProfile(const OutputProfile &profile);

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
    SongPanel *m_songPanel;
    ThemePanel *m_themePanel;
    QTreeWidget *m_categoryTree;
    QListWidget *m_mediaGrid;
    LiveAppearancePreview *m_preview;
    QLabel *m_itemCountLabel;
    QLineEdit *m_searchBox;
    QToolButton *m_importButton;

    QString m_currentCategory;
    QMap<QString, QVector<MediaEntry>> m_sampleData;
};

#endif // SANCTIFYLIVE_CORE_MEDIALIBRARYPANEL_H_
