#pragma once

#include <QColor>
#include <QMainWindow>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QToolButton;
class QLabel;
class QLineEdit;
class QTabWidget;
class QFrame;
class QToolBar;
class ScheduleModel;
class OutputWindow;
class SlideCanvas;
class MediaLibraryPanel;
class ScripturePanel;
class SongsPanel;
class LiveCaptionsPanel;
class VerseDetectionPanel;

// =====================================================================
// PANEL GLOSSARY -- read this before touching layout code.
//
// Every named box the operator sees has exactly one canonical name,
// used consistently in code, comments, and conversation so "move the
// preview" is never ambiguous. Two different things are both called
// "preview" on purpose (see PREVIEW vs ITEM PREVIEW below) -- that's the
// one wrinkle worth memorizing; everything else maps 1:1.
//
//   TOOLBAR           Top bar: New/Open/Save, Web/Remote (stubs),
//                     Go Live, Alerts/Logo (stubs), Black, Clear, the
//                     LIVE/Offline indicator. Built by buildToolBar().
//   MENU BAR          File/Live/Profiles/View/Help. Every entry
//                     duplicates a Toolbar/panel action -- nothing is
//                     menu-only. Built by buildMenuBar().
//
//   MAIN ROW          The three panels below the Toolbar, left to
//                     right. Built by buildMainRow().
//     SCHEDULE          The run order for the service (OpenLP calls its
//                       equivalent the "Service Manager"). A single
//                       click stages a row in Preview; double-click (or
//                       keyboard shortcuts) commits it Live. Starts
//                       empty -- there's no manual "type a slide" flow;
//                       items arrive by picking real content elsewhere
//                       (e.g. double-clicking a Media tile). List
//                       widget: m_scheduleList. Built by
//                       buildSchedulePanel().
//     PREVIEW           Shows whichever Schedule row is currently
//                       *selected* -- staged, not yet live. Canvas:
//                       m_previewCanvas. This is a SERVICE preview
//                       (what would go live next), NOT the same thing
//                       as Item Preview below.
//     LIVE              Mirrors the model's actual live position at all
//                       times, in-app. Whether the congregation-facing
//                       Output Window is actually showing it is a
//                       separate question -- see "Go Live" in the class
//                       doc comment below. Canvas: m_livePreview (an
//                       embedded OutputWindow instance).
//
//   TRANSPORT ROW     Just the "Next: ..." label now -- purely
//                     informational. Advancing/retreating happens via a
//                     Schedule double-click or keyboard shortcuts
//                     (arrow keys/space/B; see keyPressEvent()), not a
//                     dedicated Previous/Next button pair. Between the
//                     Main Row and the Lower Area. Built by
//                     buildTransportRow().
//
//   LOWER AREA        Everything below the Transport Row, three columns
//                     left to right: Content Tabs, Item Preview, and a
//                     History-over-Transcription column. Built by
//                     buildLowerArea().
//     CONTENT TABS      The Songs/Scriptures/Media/Presentations/Themes
//                       tab strip -- where the operator browses source
//                       material to add to Schedule. Built by
//                       buildContentTabs(). The Media tab's own
//                       internals (folder tree + thumbnail grid) are
//                       MediaLibraryPanel's job, and the Scriptures
//                       tab's own internals (translation checklist +
//                       verse table) are ScripturePanel's job -- not
//                       this class's -- see each header's own glossary
//                       note.
//     ITEM PREVIEW      The middle column -- sits in the gap between
//                       Content Tabs and History, not wedged inside any
//                       one Content Tab (see MediaLibraryPanel's own
//                       glossary note). Shows a pixel-faithful render
//                       of whatever's currently selected/hovered in
//                       Content Tabs (Media today; Songs/Presentations/
//                       Themes once they're real) -- BEFORE it's added
//                       to Schedule. Canvas: m_itemPreviewCanvas. This
//                       is a LIBRARY-CONTENT preview, NOT the same
//                       thing as the Main Row's Preview panel (which
//                       previews the service run order).
//     HISTORY           Rightmost column, top half. Read-only, append-
//                       only, most-recent-first log of every slide that
//                       has actually gone live, timestamped. Distinct
//                       from Schedule (the plan) -- this is the record.
//                       List widget: m_historyList. Built by
//                       buildHistoryPanel().
//     TRANSCRIPTION     Rightmost column, bottom half, below History.
//                       LiveCaptionsPanel (m_liveCaptionsPanel) is a
//                       real, working scrolling caption display; what's
//                       still missing is anything to feed it -- no
//                       audio capture/speech-to-text pipeline exists
//                       yet (see README roadmap), so Start Transcription
//                       stays an honest disabled stub. Below it,
//                       VerseDetectionPanel (m_verseDetectionPanel) is
//                       fully real and working: it shows whatever
//                       core/scripture/ScriptureDetector found in the
//                       most recent transcribed line, click to preview
//                       or double-click to add -- the same detect-a-
//                       spoken-reference-and-offer-to-project-it feature
//                       tools like Pewbeam center their whole product
//                       on. Since there's no live audio yet, a
//                       "Simulate Caption" field (m_simulateCaptionEdit)
//                       feeds it a typed test line through the exact
//                       same LiveCaptionsPanel::appendCaption() path a
//                       real STT engine would eventually call, so
//                       detection is genuinely exercised today, not
//                       just wired up for later. Built by
//                       buildTranscriptionPanel().
//
//   STATUS BAR        Bottom-of-window strip: output state + slide
//                     count on the left, "Wake Display" button on the
//                     right. Set up inline in the constructor.
//
// OUTPUT WINDOW (a separate class, ui/OutputWindow.h) is the actual,
// possibly-fullscreen, congregation-facing display -- what a projector
// shows. m_outputWindow is that real window; m_livePreview is a second,
// embedded OutputWindow instance used only to render the Live panel
// in-app. Both watch the same ScheduleModel, so they can never disagree
// about what's live.
// =====================================================================

// OperatorWindow is the volunteer-facing control surface. Visually it
// follows a familiar broadcast-console layout, closer to how OpenLP
// arranges its Service Manager / Preview / Live panels than to a single
// stacked list. See the glossary above for what every panel is called;
// this comment is about how they behave together.
//
// One ScheduleModel is the single source of truth, and everything here
// either reads from it or sends it a command. The only piece of state
// that lives in this class (deliberately NOT in ScheduleModel) is which
// Schedule row is currently *previewed*. This mirrors OpenLP's Service
// Manager behavior: a single click on a Schedule row only stages it in
// Preview; a double-click (or keyboard shortcuts) commits it live. The
// in-app Live panel always mirrors the model's live position
// regardless of whether the real Output Window is on screen -- "Go Live"
// only controls whether that live content is actually being sent to the
// congregation-facing display, not whether the model's live position
// changes. So: if Go Live is on, staging something in Preview and
// activating it puts it on the real output; if Go Live is off, the same
// action still updates the model (and the in-app Live panel still
// reflects it truthfully) but nothing is actually shown to the
// congregation until Go Live is switched on.
class OperatorWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OperatorWindow(QWidget *parent = nullptr);
    ~OperatorWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onScheduleItemSelected(int row);
    void onScheduleItemActivated(QListWidgetItem *item);
    void onLiveContentChanged();
    void onScheduleChanged();
    void onHistoryChanged();
    void onToggleOutputWindow(bool checked);
    void onClearClicked();
    void onWakeDisplayClicked();
    void onNewSchedule();
    void onOpenSchedule();
    void onSaveSchedule();
    void onMediaActivated(const QString &name, const QColor &color, const QString &imagePath);
    void onMediaPreviewRequested(const QString &name, const QColor &color, const QString &imagePath);
    void onScriptureActivated(const QString &reference, const QString &text);
    void onScripturePreviewRequested(const QString &reference, const QString &text);
    void onScripturePreviewCleared();
    void onSongSectionActivated(const QString &label, const QString &text);
    void onSongPreviewRequested(const QString &label, const QString &text);
    void onWholeSongActivated(const QStringList &labels, const QStringList &texts);
    void onSimulateCaptionSubmitted();

private:
    QToolBar *buildToolBar();
    void buildMenuBar(QToolBar *toolBar);
    QWidget *buildMainRow();
    QWidget *buildTransportRow();
    QWidget *buildLowerArea();
    QWidget *buildSchedulePanel();
    QWidget *buildItemPreviewPanel();
    QWidget *buildHistoryPanel();
    QWidget *buildTranscriptionPanel();
    QTabWidget *buildContentTabs();
    QFrame *wrapInPanelCard(const QString &title, QWidget *content, const char *titleObjectName = "panelTitle");

    void rebuildScheduleList();
    void rebuildHistoryList();
    void updatePreviewForRow(int row);
    void updateNextSlideLabel();
    void updateLiveIndicator();
    void updateStatusBar();

    ScheduleModel *m_model;
    OutputWindow *m_outputWindow;  // OUTPUT WINDOW: the actual congregation-facing window
    OutputWindow *m_livePreview;   // LIVE panel: small in-window mirror of live output
    SlideCanvas *m_previewCanvas;  // PREVIEW panel: shows whichever Schedule row is selected

    QListWidget *m_scheduleList;   // SCHEDULE panel: the run order for the service (OpenLP: "Service Manager")
    QListWidget *m_historyList;    // HISTORY panel: append-only log of what has actually gone live
    SlideCanvas *m_itemPreviewCanvas; // ITEM PREVIEW panel: content-library preview, separate from PREVIEW above
    QLabel *m_itemPreviewLabel;       // ITEM PREVIEW panel: name of whatever's currently shown there
    QLabel *m_nextSlideLabel;
    QLabel *m_liveIndicator;
    QLabel *m_statusLabel;

    QToolButton *m_blackButton;
    QToolButton *m_clearButton;
    QPushButton *m_goLiveButton;

    MediaLibraryPanel *m_mediaPanel;
    ScripturePanel *m_scripturePanel;
    SongsPanel *m_songsPanel;
    LiveCaptionsPanel *m_liveCaptionsPanel; // TRANSCRIPTION panel: scrolling live-speech display
    VerseDetectionPanel *m_verseDetectionPanel; // shows what ScriptureDetector found in the latest transcribed line
    QLineEdit *m_simulateCaptionEdit; // testing aid -- see buildTranscriptionPanel()
};
