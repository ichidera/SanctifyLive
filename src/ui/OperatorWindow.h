#pragma once

#include <QColor>
#include <QMainWindow>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QToolButton;
class QLabel;
class QTabWidget;
class QFrame;
class QToolBar;
class ScheduleModel;
class OutputWindow;
class SlideCanvas;
class MediaLibraryPanel;

// OperatorWindow is the volunteer-facing control surface. Visually it
// follows a familiar broadcast-console layout, closer to how OpenLP
// arranges its Service Manager / Preview / Live panels than to a single
// stacked list:
//
//   - A top toolbar for file/session actions and the live transport.
//   - A main row of three panels side by side: Schedule (the run order
//     for the service -- OpenLP calls its equivalent the "Service
//     Manager"), Preview (whatever Schedule row is currently selected,
//     staged but not yet live), and Live (a mirror of the actual
//     congregation-facing output).
//   - A bottom tab strip for browsing content (Songs/Scriptures/Media/
//     Presentations/Themes) alongside a History panel (an append-only
//     log of what has actually gone live) and a Transcription panel
//     (a placeholder stub -- see README roadmap).
//
// Functionally this is still the same control loop as before: one
// ScheduleModel is the single source of truth, and everything here
// either reads from it or sends it a command. The only piece of state
// that lives in this class (deliberately NOT in ScheduleModel) is which
// Schedule row is currently *previewed*. This mirrors OpenLP's Service
// Manager behavior: a single click on a Schedule row only stages it in
// Preview; a double-click (or Next/Previous/keyboard shortcuts) commits
// it live. The in-app Live pane always mirrors the model's live position
// regardless of whether the real output window is on screen -- "Go Live"
// only controls whether that live content is actually being sent to the
// congregation-facing display, not whether the model's live position
// changes. So: if Go Live is on, staging something in Preview and
// activating it puts it on the real output; if Go Live is off, the same
// action still updates the model (and the in-app Live pane still
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
    void onAddSlideClicked();
    void onRemoveSlideClicked();
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
    void onMediaActivated(const QString &name, const QColor &color);

private:
    QToolBar *buildToolBar();
    void buildMenuBar(QToolBar *toolBar);
    QWidget *buildMainRow();
    QWidget *buildTransportRow();
    QWidget *buildLowerArea();
    QWidget *buildSchedulePanel();
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
    OutputWindow *m_outputWindow;  // the actual congregation-facing window
    OutputWindow *m_livePreview;   // small in-window mirror of live output
    SlideCanvas *m_previewCanvas;  // shows whichever Schedule row is selected

    QListWidget *m_scheduleList;   // the run order for the service (OpenLP: "Service Manager")
    QListWidget *m_historyList;    // append-only log of what has actually gone live
    QLabel *m_nextSlideLabel;
    QLabel *m_liveIndicator;
    QLabel *m_statusLabel;

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QToolButton *m_blackButton;
    QToolButton *m_clearButton;
    QPushButton *m_goLiveButton;

    MediaLibraryPanel *m_mediaPanel;
};
