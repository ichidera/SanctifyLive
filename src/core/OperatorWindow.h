#ifndef SANCTIFYLIVE_CORE_OPERATORWINDOW_H_
#define SANCTIFYLIVE_CORE_OPERATORWINDOW_H_


#include <QMainWindow>

class QListWidget;
class QListWidgetItem;
class QLabel;
class ScheduleModel;
class OutputWindow;
class MediaLibraryPanel;
class HistoryBar;

// OperatorWindow: menu bar + toolbar up top, a History strip, then a
// resizable vertical split between the three-column live workspace
// (Schedule | Live editor | Live Output) and the bottom media/resource
// library. Every internal division is a QSplitter and every major panel
// has a View-menu toggle, so the layout is fully rearrangeable.
//
// Interaction model: selecting a Schedule item, or double-clicking a
// media item, commits straight to Live -- there's no separate "staged"
// step. Sending media live does NOT add it to the Schedule by default
// (the Schedule is the pre-planned service order, not a log of what's
// been shown); that can be turned on in Edit > Options if someone wants
// the old behavior. Whatever goes live -- from the Schedule or the
// library -- is recorded in the History strip regardless, so the
// operator can always get back to something they showed.
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
    void onScheduleRowChanged(int row);
    void onMediaActivated(const QString &label, const QColor &background, const QString &imagePath);
    void onLiveContentChanged();
    void onScheduleChanged();

    void onNewSchedule();
    void onGoLiveAction();
    void onBlackToggled(bool checked);
    void onClearAction();
    void onLiveOutputToggled(bool checked);
    void onEditOptions();
    void onAbout();

private:
    void buildMenuBar();
    void buildToolBar();
    QWidget *buildWorkspace();

    void rebuildScheduleList();
    void refreshScheduleHighlighting();
    void refreshLiveOutputFooter();
    void showOutputWindow();
    void hideOutputWindow();

    ScheduleModel *m_model;
    OutputWindow *m_outputWindow;   // real congregation-facing window
    OutputWindow *m_liveEditorView; // "Live - <item>" mirror (middle pane)
    OutputWindow *m_liveOutputView; // "Live Output" mirror (right pane)
    MediaLibraryPanel *m_mediaLibrary;
    HistoryBar *m_historyBar;

    // Panel containers, kept as members so the View menu can toggle them.
    QWidget *m_schedulePanel;
    QWidget *m_liveEditorPanel;
    QWidget *m_liveOutputPanel;

    QListWidget *m_scheduleList;
    QLabel *m_liveEditorHeader;
    QLabel *m_slideCounterLabel;

    QAction *m_actBlack;
    QAction *m_actLive;
};

#endif // SANCTIFYLIVE_CORE_OPERATORWINDOW_H_