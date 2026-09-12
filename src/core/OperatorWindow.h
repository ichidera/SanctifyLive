#pragma once

#include <QMainWindow>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLabel;
class QTabWidget;
class ScheduleModel;
class OutputWindow;

// OperatorWindow is the volunteer-facing control surface. Layout follows
// the EasyWorship/ProPresenter convention deliberately:
//
//   [ Resource tabs: Schedule | Songs | Media | Scripture ]
//   [ Schedule list ]   [ PREVIEW pane + Go Live ]   [ LIVE pane ]
//   [                Transport: Prev / Next / Black                ]
//
// Songs/Media/Scripture tabs are visible-but-disabled placeholders for
// now -- they exist so the panel structure is already correct when
// those libraries are built (see build-plan steps 4-6), rather than
// requiring a layout rework later.
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
    void onScheduleRowChanged(int row);          // selection -> preview only
    void onScheduleItemDoubleClicked(QListWidgetItem *item); // -> go live
    void onGoLiveClicked();
    void onLiveContentChanged();
    void onPreviewChanged();
    void onScheduleChanged();
    void onToggleOutputWindow();

private:
    void rebuildScheduleList();
    void refreshScheduleHighlighting();

    ScheduleModel *m_model;
    OutputWindow *m_outputWindow;   // real congregation-facing window (Live)
    OutputWindow *m_livePreview;    // small in-app mirror of Live
    OutputWindow *m_previewPane;    // staged-but-not-live preview

    QTabWidget *m_resourceTabs;
    QListWidget *m_scheduleList;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;

    QPushButton *m_goLiveButton;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QPushButton *m_blackButton;
    QPushButton *m_toggleOutputButton;
};