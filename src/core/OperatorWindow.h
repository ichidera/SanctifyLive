#pragma once

#include <QMainWindow>

class QListWidget;
class QListWidgetItem;
class QLabel;
class ScheduleModel;
class OutputWindow;
class MediaLibraryPanel;

// OperatorWindow: menu bar + toolbar up top, then a resizable vertical
// split between the three-column live workspace (Schedule | Live editor
// | Live Output) and the bottom media/resource library. Every internal
// division is a QSplitter and every major panel has a View-menu toggle,
// so the layout is fully rearrangeable rather than fixed.
//
// Interaction model: selecting an item in the Schedule list (or
// double-clicking a media item in the library) commits it straight to
// Live -- there is no separate "staged but not live" step, matching the
// direct schedule-builder workflow this UI is modeled on. The
// underlying ScheduleModel still supports a two-step preview/commit
// (setPreviewIndex + goLiveWithPreview) if a safer staged workflow is
// wanted later; this window just chooses to call both together.
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
    void onMediaActivated(const QString &label, const QColor &background);
    void onLiveContentChanged();
    void onScheduleChanged();

    void onNewSchedule();
    void onGoLiveAction();
    void onBlackToggled(bool checked);
    void onClearAction();
    void onLiveOutputToggled(bool checked);
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