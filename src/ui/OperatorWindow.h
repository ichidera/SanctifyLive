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
// follows a familiar broadcast-console layout: a top toolbar for
// file/session actions and the live transport, a Preview/Live pane pair
// so the operator always sees "what's selected" next to "what the
// congregation sees right now," a bottom tab strip for browsing content
// (Songs/Scriptures/Media/Presentations/Themes), and a Queue panel
// holding the run order for the current service.
//
// Functionally this is still the same control loop as before: one
// ScheduleModel is the single source of truth, and everything here
// either reads from it or sends it a command. The only new piece of
// state that lives in this class (deliberately NOT in ScheduleModel) is
// which Queue row is currently *previewed* -- selecting a row no longer
// immediately goes live, matching how the reference layout separates
// Preview from Live. Advancing/retreating (buttons, arrow keys, space)
// still drives the live position directly, exactly as before.
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
    void onQueueItemSelected(int row);
    void onQueueItemActivated(QListWidgetItem *item);
    void onLiveContentChanged();
    void onScheduleChanged();
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
    QWidget *buildPreviewLiveRow();
    QWidget *buildTransportRow();
    QWidget *buildLowerArea();
    QWidget *buildQueuePanel();
    QTabWidget *buildContentTabs();
    QFrame *wrapInPanelCard(const QString &title, QWidget *content, const char *titleObjectName = "panelTitle");

    void rebuildQueueList();
    void updatePreviewForRow(int row);
    void updateNextSlideLabel();
    void updateLiveIndicator();
    void updateStatusBar();

    ScheduleModel *m_model;
    OutputWindow *m_outputWindow;  // the actual congregation-facing window
    OutputWindow *m_livePreview;   // small in-window mirror of live output
    SlideCanvas *m_previewCanvas;  // shows whichever Queue row is selected

    QListWidget *m_queueList;
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
