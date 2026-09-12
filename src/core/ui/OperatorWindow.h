#pragma once

#include <QMainWindow>

class QListWidget;
class QPushButton;
class QLabel;
class ScheduleModel;
class OutputWindow;

// OperatorWindow is the volunteer-facing control surface: schedule list
// on the left, live/next preview in the middle, and the small set of
// controls that get used every single service (advance, back, black).
//
// Deliberately absent from this first milestone: song/Scripture
// databases, media import, themes. Those attach to ScheduleModel later
// without this class needing structural changes -- the point of this
// pass is to prove the live control loop (see build-plan step 3) with
// plain text slides before anything else is layered on.
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
    void onScheduleItemActivated(int row);
    void onLiveContentChanged();
    void onScheduleChanged();
    void onToggleOutputWindow();

private:
    void rebuildScheduleList();
    void updatePreviewLabels();

    ScheduleModel *m_model;
    OutputWindow *m_outputWindow;   // the actual congregation-facing window
    OutputWindow *m_livePreview;    // small in-window mirror (no second monitor required)

    QListWidget *m_scheduleList;
    QLabel *m_nextSlideLabel;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QPushButton *m_blackButton;
    QPushButton *m_toggleOutputButton;
};