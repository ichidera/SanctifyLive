#include "core/ui/OperatorWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "core/model/ScheduleModel.h"
#include "core/ui/OutputWindow.h"
#include "core/ui/Theme.h"
#include "core/ui/panels/LibraryPanel.h"
#include "core/ui/panels/LiveCaptionsPanel.h"
#include "core/ui/panels/QueuePanel.h"

OperatorWindow::OperatorWindow(QWidget *parent)
    : QMainWindow(parent), m_model(new ScheduleModel(this))
{
    setWindowTitle(tr("SanctifyLive - Operator"));
    resize(1400, 820);

    // Three OutputWindow instances share the same model: the real
    // congregation-facing output (Live source), an embedded "Live" pane
    // so the operator can confirm what's on air without a second
    // monitor, and an embedded "Preview" pane showing whatever is
    // currently staged. All three are the same class rendering the same
    // way -- see OutputWindow's class comment.
    m_outputWindow = new OutputWindow(m_model, OutputWindow::Source::Live, nullptr);
    m_livePreview = new OutputWindow(m_model, OutputWindow::Source::Live, this);
    m_previewPane = new OutputWindow(m_model, OutputWindow::Source::Preview, this);
    m_livePreview->setMinimumHeight(160);
    m_previewPane->setMinimumHeight(160);

    buildToolBar();
    buildCentralArea();
    buildStatusBar();

    connect(m_model, &ScheduleModel::liveContentChanged, this, [this] {
        m_liveToggleButton->setChecked(m_outputWindow->isVisible());
    });

    // A couple of starter slides so the app isn't empty on first launch.
    m_model->addSlide(Slide(tr("Psalm 91:1 [NLT]"),
                             tr("1. He that dwelleth in the secret place of the most "
                                "High shall abide under the shadow of the Almighty."),
                             QColor("#0f1b33")));
    m_model->addSlide(Slide(tr("Announcement"),
                             tr("Coffee & fellowship after the service"),
                             QColor("#16213e")));

    setFocusPolicy(Qt::StrongFocus);
}

OperatorWindow::~OperatorWindow()
{
    // m_outputWindow has no parent (it's a real top-level window so it can
    // be moved to a second monitor independent of the operator window),
    // so it won't be destroyed automatically by Qt's parent/child cleanup.
    delete m_outputWindow;
}

void OperatorWindow::buildToolBar()
{
    auto *toolBar = new QToolBar(tr("Main"), this);
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->setIconSize(QSize(20, 20));
    addToolBar(toolBar);

    QStyle *style = this->style();

    // --- Left group: project-level actions. None of these have a real
    // backend yet (no file format, no asset store, no network layer) --
    // they're wired to an honest "not implemented" message rather than
    // silently doing nothing, so clicking one is never confusing.
    auto addPlaceholderAction = [&](QStyle::StandardPixmap icon, const QString &text) {
        QAction *action = toolBar->addAction(style->standardIcon(icon), text);
        connect(action, &QAction::triggered, this, [this, text] { onNotImplemented(text); });
        return action;
    };

    QAction *newAction = toolBar->addAction(style->standardIcon(QStyle::SP_FileIcon), tr("New"));
    connect(newAction, &QAction::triggered, this, &OperatorWindow::onNewClicked);

    addPlaceholderAction(QStyle::SP_DialogOpenButton, tr("Open"));
    addPlaceholderAction(QStyle::SP_DialogSaveButton, tr("Save"));
    addPlaceholderAction(QStyle::SP_DriveHDIcon, tr("Store"));
    addPlaceholderAction(QStyle::SP_DriveNetIcon, tr("Web"));
    addPlaceholderAction(QStyle::SP_ComputerIcon, tr("Remote"));

    // Push everything after this to the right edge of the toolbar.
    auto *spacer = new QWidget(toolBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);

    // --- Right group: live-control actions ---
    auto *goLiveButton = new QPushButton(tr("Go Live"), toolBar);
    goLiveButton->setStyleSheet(QString("QPushButton { background-color: %1; color: white; "
                                         "font-weight: 700; border-radius: 6px; padding: 6px 16px; "
                                         "border: none; } QPushButton:hover { background-color: #c23333; }")
                                    .arg(Theme::Colors::accentRecord));
    connect(goLiveButton, &QPushButton::clicked, m_model, &ScheduleModel::goLive);
    toolBar->addWidget(goLiveButton);

    addPlaceholderAction(QStyle::SP_MessageBoxWarning, tr("Alerts"));
    addPlaceholderAction(QStyle::SP_DesktopIcon, tr("Logo"));

    QAction *blackAction =
        toolBar->addAction(style->standardIcon(QStyle::SP_DialogNoButton), tr("Black"));
    blackAction->setCheckable(true);
    connect(blackAction, &QAction::toggled, m_model, &ScheduleModel::setBlackout);
    m_blackButton = qobject_cast<QToolButton *>(toolBar->widgetForAction(blackAction));
    if (m_blackButton) {
        m_blackButton->setStyleSheet(
            QString("QToolButton:checked { background-color: %1; color: white; }")
                .arg(Theme::Colors::accentRecord));
    }

    QAction *clearAction =
        toolBar->addAction(style->standardIcon(QStyle::SP_DialogResetButton), tr("Clear"));
    connect(clearAction, &QAction::triggered, m_model, &ScheduleModel::clearPreview);

    QAction *liveAction = toolBar->addAction(tr("\u25CF Live"));
    liveAction->setCheckable(true);
    connect(liveAction, &QAction::toggled, this, &OperatorWindow::onToggleOutputWindow);
    m_liveToggleButton = qobject_cast<QToolButton *>(toolBar->widgetForAction(liveAction));
    if (m_liveToggleButton) {
        m_liveToggleButton->setStyleSheet(
            QString("QToolButton:checked { background-color: %1; color: black; }")
                .arg(Theme::Colors::accentLive));
    }

    // keyPressEvent's 'B' shortcut routes through this button so its
    // checked state and the toolbar's visual state never disagree.
    m_blackButtonAction = blackAction;
}

void OperatorWindow::buildCentralArea()
{
    // --- Top row: Live Captions | Preview | Live ---
    m_captionsPanel = new LiveCaptionsPanel(this);
    m_captionsPanel->setMinimumWidth(220);

    auto *previewTitle = new QLabel(tr("Preview"), this);
    previewTitle->setProperty("role", "sectionTitle");
    auto *previewColumn = new QWidget(this);
    auto *previewLayout = new QVBoxLayout(previewColumn);
    previewLayout->addWidget(previewTitle);
    previewLayout->addWidget(m_previewPane, 1);

    auto *liveTitle = new QLabel(tr("Live"), this);
    liveTitle->setProperty("role", "liveTitle");
    auto *liveColumn = new QWidget(this);
    auto *liveLayout = new QVBoxLayout(liveColumn);
    liveLayout->addWidget(liveTitle);
    liveLayout->addWidget(m_livePreview, 1);

    auto *topSplitter = new QSplitter(this);
    topSplitter->addWidget(m_captionsPanel);
    topSplitter->addWidget(previewColumn);
    topSplitter->addWidget(liveColumn);
    topSplitter->setStretchFactor(0, 1);
    topSplitter->setStretchFactor(1, 2);
    topSplitter->setStretchFactor(2, 2);

    // --- Bottom row: Library | Queue ---
    m_libraryPanel = new LibraryPanel(this);
    m_queuePanel = new QueuePanel(m_model, this);
    connect(m_queuePanel, &QueuePanel::addSlideRequested,
            this, &OperatorWindow::onAddSlideRequested);

    auto *bottomSplitter = new QSplitter(this);
    bottomSplitter->addWidget(m_libraryPanel);
    bottomSplitter->addWidget(m_queuePanel);
    bottomSplitter->setStretchFactor(0, 2);
    bottomSplitter->setStretchFactor(1, 1);

    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(bottomSplitter);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    setCentralWidget(mainSplitter);
}

void OperatorWindow::buildStatusBar()
{
    auto *stageLabel = new QLabel(tr("Stage Display: not configured  \u2022  0 devices connected"), this);
    stageLabel->setProperty("role", "muted");
    statusBar()->addWidget(stageLabel, 1);

    auto *wakeButton = new QPushButton(tr("Wake Display"), this);
    wakeButton->setEnabled(false); // no stage-display network layer yet
    wakeButton->setToolTip(tr("Requires a paired Stage Display device (not implemented yet)"));
    statusBar()->addPermanentWidget(wakeButton);
}

void OperatorWindow::onAddSlideRequested()
{
    bool ok = false;
    const QString text = QInputDialog::getMultiLineText(
        this, tr("Add Slide"), tr("Slide text:"), QString(), &ok);
    if (!ok || text.trimmed().isEmpty())
        return;

    // Use the first line as the queue-card label so the operator can
    // scan the list without the full text cluttering it.
    const QString label = text.section('\n', 0, 0).left(40);
    m_model->addSlide(Slide(label, text));
}

void OperatorWindow::onNewClicked()
{
    if (m_model->count() == 0)
        return;

    const auto choice = QMessageBox::question(
        this, tr("New Schedule"),
        tr("Clear the current queue and start a new schedule?"),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (choice == QMessageBox::Yes)
        m_model->clearAll();
}

void OperatorWindow::onNotImplemented(const QString &feature)
{
    statusBar()->showMessage(tr("%1 isn't implemented yet -- see the README roadmap.")
                                  .arg(feature),
                              4000);
}

void OperatorWindow::onToggleOutputWindow()
{
    if (m_outputWindow->isVisible()) {
        m_outputWindow->hide();
        return;
    }

    // If a second screen is connected, send the real output there and go
    // fullscreen -- that's the whole point of a projector/confidence-
    // monitor setup. Otherwise fall back to a normal window so this is
    // still usable on a single-monitor dev machine.
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.size() > 1) {
        QScreen *outputScreen = screens.at(1);
        m_outputWindow->setGeometry(outputScreen->geometry());
        m_outputWindow->showFullScreen();
    } else {
        m_outputWindow->resize(960, 540);
        m_outputWindow->show();
    }
}

void OperatorWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    // Whenever the operator window becomes the active window again (e.g.
    // the user alt-tabs back to it, or clicks it after having another app
    // on top), bring the output window back above whatever was covering
    // it. raise() only reorders stacking -- it does NOT steal keyboard
    // focus, so the operator window stays the one receiving keypresses
    // (arrow keys / space / B still work uninterrupted).
    if (event->type() == QEvent::ActivationChange && isActiveWindow()) {
        if (m_outputWindow->isVisible())
            m_outputWindow->raise();
    }
}

void OperatorWindow::closeEvent(QCloseEvent *event)
{
    // Treat this as one application: closing the control window should
    // never leave a bare output window orphaned on screen (or keep the
    // process alive because a top-level window is technically still
    // open). Close it explicitly before the base implementation proceeds.
    m_outputWindow->close();
    QMainWindow::closeEvent(event);
}

void OperatorWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Space:
    case Qt::Key_Down:
        m_model->advance();
        break;
    case Qt::Key_Left:
    case Qt::Key_Up:
        m_model->retreat();
        break;
    case Qt::Key_B:
        if (m_blackButtonAction)
            m_blackButtonAction->toggle(); // routes through the action so toolbar state stays in sync
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }
    event->accept();
}
