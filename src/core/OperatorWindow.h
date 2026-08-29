#ifndef SANCTIFYLIVE_CORE_OPERATORWINDOW_H_
#define SANCTIFYLIVE_CORE_OPERATORWINDOW_H_


#include <QMainWindow>
#include <QPointF>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QPushButton;
class ScheduleModel;
class OutputWindow;
class MediaLibraryPanel;
class HistoryBar;
class SlideServer;
class SettingsWindow;

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
//
// This window also owns the SlideServer, which mirrors live content to
// any connected Android stage-display devices over the local network
// (see src/android/PROTOCOL.md and src/core/SlideServer.h). A status
// bar label shows the listening address and how many devices are
// connected, alongside a "Wake Display" button (enabled only while a
// device is connected) that forces a sleeping/locked tablet's screen
// back on -- for when it's timed out or been put to sleep and the
// operator would otherwise have to walk over and tap it.
//
// Connected devices are one of two roles -- "display" (sanctuary/stage
// screens, always mirroring live) or "phone" (an operator's handheld,
// which can be pinned to different content via "Send to Phone Only" in
// the Media Library, or cleared back to mirroring via the Live menu).
// Edit > Options opens SettingsWindow, a persistent (non-modal) window
// with a Displays tab listing exactly what's connected and as what.
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
    void onMediaActivated(const QString &label, const QColor &background, const QString &imagePath,
                          const QPointF &focus);
    void onMediaSentToPhone(const QString &label, const QColor &background, const QString &imagePath,
                            const QPointF &focus);
    void onScriptureActivated(const QString &reference, const QString &text, const QString &translationCode);
    void onLiveContentChanged();
    void onScheduleChanged();

    void onNewSchedule();
    void onGoLiveAction();
    void onBlackToggled(bool checked);
    void onClearAction();
    void onLiveOutputToggled(bool checked);
    void onEditOptions();
    void onAbout();
    void onDisplayClientCountChanged(int count);
    void onWakeDisplayClicked();
    void onSendLiveToPhoneOnly();
    void onMirrorPhoneToMain();

private:
    void buildMenuBar();
    void buildToolBar();
    QWidget *buildWorkspace();

    void rebuildScheduleList();
    void refreshScheduleHighlighting();
    void refreshLiveOutputFooter();
    void showOutputWindow();
    void hideOutputWindow();
    QString localNetworkStatusText(int clientCount) const;

    ScheduleModel *m_model;
    OutputWindow *m_outputWindow;   // real congregation-facing window
    OutputWindow *m_liveEditorView; // "Live - <item>" mirror (middle pane)
    OutputWindow *m_liveOutputView; // "Live Output" mirror (right pane)
    MediaLibraryPanel *m_mediaLibrary;
    HistoryBar *m_historyBar;
    SlideServer *m_slideServer;
    SettingsWindow *m_settingsWindow = nullptr; // lazily created on first Edit > Options

    // Panel containers, kept as members so the View menu can toggle them.
    QWidget *m_schedulePanel;
    QWidget *m_liveEditorPanel;
    QWidget *m_liveOutputPanel;

    QListWidget *m_scheduleList;
    QLabel *m_liveEditorHeader;
    QLabel *m_slideCounterLabel;
    QLabel *m_networkStatusLabel;
    QPushButton *m_wakeDisplayButton; // status bar; enabled only while >=1 device connected

    QAction *m_actBlack;
    QAction *m_actLive;
};

#endif // SANCTIFYLIVE_CORE_OPERATORWINDOW_H_
