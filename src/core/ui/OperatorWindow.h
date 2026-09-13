#pragma once

#include <QMainWindow>

class QToolBar;
class QLabel;
class QPushButton;
class QToolButton;
class QAction;
class ScheduleModel;
class OutputWindow;
class QueuePanel;
class LibraryPanel;
class LiveCaptionsPanel;

// OperatorWindow is the volunteer-facing control surface. Layout mirrors
// the reference design: a toolbar of app-wide actions across the top, a
// captions/Preview/Live row, a Library/Queue row, and a status bar for
// output/connection state at the bottom.
//
// Everything here is a thin shell around ScheduleModel plus a handful of
// self-contained panels (QueuePanel, LibraryPanel, LiveCaptionsPanel) --
// this class's job is wiring those pieces together and owning the
// top-level window chrome (toolbar, status bar, keyboard shortcuts,
// output-window lifecycle), not implementing any of their behavior
// itself.
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
    void onAddSlideRequested();
    void onToggleOutputWindow();
    void onNewClicked();
    void onNotImplemented(const QString &feature);

private:
    void buildToolBar();
    void buildCentralArea();
    void buildStatusBar();

    ScheduleModel *m_model;
    OutputWindow *m_outputWindow; // the actual congregation-facing window
    OutputWindow *m_livePreview;  // embedded "Live" pane
    OutputWindow *m_previewPane;  // embedded "Preview" (staged) pane

    QueuePanel *m_queuePanel;
    LibraryPanel *m_libraryPanel;
    LiveCaptionsPanel *m_captionsPanel;

    QToolButton *m_blackButton;
    QToolButton *m_liveToggleButton;
    QAction *m_blackButtonAction = nullptr;
};
