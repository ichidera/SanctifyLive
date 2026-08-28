#include "OperatorWindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

#include "HistoryBar.h"
#include "IconFactory.h"
#include "MediaLibraryPanel.h"
#include "OutputWindow.h"
#include "ScheduleModel.h"
#include "SlideServer.h"

namespace {
QLabel *makePanelHeader(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setStyleSheet("background-color: #1a1a1d; color: #cfcfcf; padding: 4px 8px; "
                          "border-bottom: 1px solid #333;");
    return label;
}

// A toolbar/menu action that's part of the visual layout but not wired
// up yet -- disabled with an explanatory tooltip rather than silently
// doing nothing, which would be a worse HCI failure than not having the
// button at all.
QAction *makeComingSoonAction(const QIcon &icon, const QString &text, QObject *parent)
{
    auto *action = new QAction(icon, text, parent);
    action->setEnabled(false);
    action->setToolTip(QObject::tr("Coming in a future update"));
    return action;
}

// Best-effort guess at the machine's LAN-facing IPv4 address, purely so
// the status bar can tell the operator what to type into the Android
// app -- see src/android/PROTOCOL.md. Not authoritative: a machine with
// multiple adapters (VPN, virtual switches, etc.) may have several
// candidates; this just picks the first plausible one rather than
// trying to be clever about routing.
QString firstLanIPv4Address()
{
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
            return address.toString();
    }
    return QObject::tr("(no network)");
}
}

OperatorWindow::OperatorWindow(QWidget *parent)
    : QMainWindow(parent), m_model(new ScheduleModel(this))
{
    setWindowTitle(tr("SanctifyLive"));
    resize(1500, 820);

    m_outputWindow = new OutputWindow(m_model, OutputWindow::Source::Live, nullptr);
    m_liveEditorView = new OutputWindow(m_model, OutputWindow::Source::Preview, this);
    m_liveOutputView = new OutputWindow(m_model, OutputWindow::Source::Live, this);

    buildMenuBar();
    buildToolBar();

    m_historyBar = new HistoryBar(m_model, this);
    setCentralWidget(buildWorkspace());

    m_slideServer = new SlideServer(m_model, this);
    connect(m_slideServer, &SlideServer::clientCountChanged,
            this, &OperatorWindow::onDisplayClientCountChanged);

    m_networkStatusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_networkStatusLabel);

    if (m_slideServer->start()) {
        onDisplayClientCountChanged(0);
    } else {
        m_networkStatusLabel->setText(tr("Display server: failed to start (port in use?)"));
    }

    connect(m_model, &ScheduleModel::scheduleChanged, this, &OperatorWindow::onScheduleChanged);
    connect(m_model, &ScheduleModel::liveContentChanged, this, &OperatorWindow::onLiveContentChanged);

    // Starter content so the app isn't empty on first launch.
    m_model->addSlide(Slide(tr("Welcome"), tr("Welcome to the service"), QColor("#1a1a2e")));
    m_model->addSlide(Slide(tr("Announcement"), tr("Coffee & fellowship after the service"), QColor("#16213e")));

    setFocusPolicy(Qt::StrongFocus);
}

OperatorWindow::~OperatorWindow()
{
    m_slideServer->stop();

    // m_outputWindow has no parent (it's a real top-level window so it can
    // be moved to a second monitor independent of the operator window),
    // so it won't be destroyed automatically by Qt's parent/child cleanup.
    delete m_outputWindow;
}

// ---------------------------------------------------------------------
// Menu bar
// ---------------------------------------------------------------------

void OperatorWindow::buildMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *actNew = fileMenu->addAction(tr("&New Schedule"));
    connect(actNew, &QAction::triggered, this, &OperatorWindow::onNewSchedule);
    fileMenu->addAction(makeComingSoonAction(QIcon(), tr("&Open..."), this));
    fileMenu->addAction(makeComingSoonAction(QIcon(), tr("&Save"), this));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(makeComingSoonAction(QIcon(), tr("&Undo"), this));
    editMenu->addAction(makeComingSoonAction(QIcon(), tr("&Redo"), this));
    editMenu->addSeparator();
    QAction *actOptions = editMenu->addAction(tr("&Options..."));
    connect(actOptions, &QAction::triggered, this, &OperatorWindow::onEditOptions);

    QMenu *liveMenu = menuBar()->addMenu(tr("&Live"));
    QAction *actGoLiveMenu = liveMenu->addAction(tr("&Go Live"));
    connect(actGoLiveMenu, &QAction::triggered, this, &OperatorWindow::onGoLiveAction);
    QAction *actBlackMenu = liveMenu->addAction(tr("&Black"));
    actBlackMenu->setCheckable(true);
    connect(actBlackMenu, &QAction::toggled, this, &OperatorWindow::onBlackToggled);
    QAction *actClearMenu = liveMenu->addAction(tr("&Clear"));
    connect(actClearMenu, &QAction::triggered, this, &OperatorWindow::onClearAction);
    // Keep the toolbar's checkable Black action and this menu action in
    // sync in both directions.
    m_actBlack = actBlackMenu;

    QMenu *profilesMenu = menuBar()->addMenu(tr("&Profiles"));
    profilesMenu->addAction(makeComingSoonAction(QIcon(), tr("Manage Profiles..."), this));

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *toggleSchedule = viewMenu->addAction(tr("Schedule Panel"));
    toggleSchedule->setCheckable(true);
    toggleSchedule->setChecked(true);
    connect(toggleSchedule, &QAction::toggled, this, [this](bool visible) { m_schedulePanel->setVisible(visible); });

    QAction *toggleLiveEditor = viewMenu->addAction(tr("Live Editor Panel"));
    toggleLiveEditor->setCheckable(true);
    toggleLiveEditor->setChecked(true);
    connect(toggleLiveEditor, &QAction::toggled, this, [this](bool visible) { m_liveEditorPanel->setVisible(visible); });

    QAction *toggleLiveOutput = viewMenu->addAction(tr("Live Output Panel"));
    toggleLiveOutput->setCheckable(true);
    toggleLiveOutput->setChecked(true);
    connect(toggleLiveOutput, &QAction::toggled, this, [this](bool visible) { m_liveOutputPanel->setVisible(visible); });

    QAction *toggleLibrary = viewMenu->addAction(tr("Media Library"));
    toggleLibrary->setCheckable(true);
    toggleLibrary->setChecked(true);
    connect(toggleLibrary, &QAction::toggled, this, [this](bool visible) { m_mediaLibrary->setVisible(visible); });

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *actAbout = helpMenu->addAction(tr("&About SanctifyLive"));
    connect(actAbout, &QAction::triggered, this, &OperatorWindow::onAbout);
}

// ---------------------------------------------------------------------
// Toolbar
// ---------------------------------------------------------------------

void OperatorWindow::buildToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolBar->setIconSize(QSize(26, 26));

    QAction *actNew = toolBar->addAction(IconFactory::newDocument(), tr("New"));
    connect(actNew, &QAction::triggered, this, &OperatorWindow::onNewSchedule);
    toolBar->addAction(makeComingSoonAction(IconFactory::openFolder(), tr("Open"), this));
    toolBar->addAction(makeComingSoonAction(IconFactory::save(), tr("Save"), this));
    toolBar->addAction(makeComingSoonAction(IconFactory::store(), tr("Store"), this));
    toolBar->addAction(makeComingSoonAction(IconFactory::web(), tr("Web"), this));
    toolBar->addAction(makeComingSoonAction(IconFactory::remote(), tr("Remote"), this));

    toolBar->addSeparator();

    auto *spacer = new QWidget(toolBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);

    QAction *actGoLive = toolBar->addAction(IconFactory::goLive(), tr("Go Live"));
    connect(actGoLive, &QAction::triggered, this, &OperatorWindow::onGoLiveAction);

    toolBar->addAction(makeComingSoonAction(IconFactory::alerts(), tr("Alerts"), this));
    toolBar->addAction(makeComingSoonAction(IconFactory::logo(), tr("Logo"), this));

    QAction *actBlack = toolBar->addAction(IconFactory::blackScreen(), tr("Black"));
    actBlack->setCheckable(true);
    connect(actBlack, &QAction::toggled, this, &OperatorWindow::onBlackToggled);
    connect(actBlack, &QAction::toggled, m_actBlack, &QAction::setChecked);
    connect(m_actBlack, &QAction::toggled, actBlack, &QAction::setChecked);

    QAction *actClear = toolBar->addAction(IconFactory::clearScreen(), tr("Clear"));
    connect(actClear, &QAction::triggered, this, &OperatorWindow::onClearAction);

    m_actLive = toolBar->addAction(IconFactory::liveMonitor(), tr("Live"));
    m_actLive->setCheckable(true);
    connect(m_actLive, &QAction::toggled, this, &OperatorWindow::onLiveOutputToggled);
}

// ---------------------------------------------------------------------
// Workspace: Schedule | Live editor | Live Output, plus the media library
// ---------------------------------------------------------------------

QWidget *OperatorWindow::buildWorkspace()
{
    // ---------- Schedule panel (left) ----------
    m_scheduleList = new QListWidget(this);
    m_scheduleList->setAlternatingRowColors(true);
    connect(m_scheduleList, &QListWidget::currentRowChanged, this, &OperatorWindow::onScheduleRowChanged);

    auto *addButton = new QPushButton(tr("+ Add Slide"), this);
    auto *removeButton = new QPushButton(tr("Remove"), this);
    connect(addButton, &QPushButton::clicked, this, &OperatorWindow::onAddSlideClicked);
    connect(removeButton, &QPushButton::clicked, this, &OperatorWindow::onRemoveSlideClicked);

    auto *scheduleButtons = new QHBoxLayout();
    scheduleButtons->addWidget(addButton);
    scheduleButtons->addWidget(removeButton);

    auto *scheduleLayout = new QVBoxLayout();
    scheduleLayout->setContentsMargins(0, 0, 0, 0);
    scheduleLayout->setSpacing(4);
    scheduleLayout->addWidget(makePanelHeader(tr("Schedule"), this));
    scheduleLayout->addWidget(m_scheduleList, 1);
    scheduleLayout->addLayout(scheduleButtons);

    m_schedulePanel = new QWidget(this);
    m_schedulePanel->setLayout(scheduleLayout);

    // ---------- Live editor panel (middle) ----------
    m_liveEditorHeader = makePanelHeader(tr("Live"), this);

    auto *liveEditorLayout = new QVBoxLayout();
    liveEditorLayout->setContentsMargins(0, 0, 0, 0);
    liveEditorLayout->setSpacing(0);
    liveEditorLayout->addWidget(m_liveEditorHeader);
    liveEditorLayout->addWidget(m_liveEditorView, 1);

    m_liveEditorPanel = new QWidget(this);
    m_liveEditorPanel->setLayout(liveEditorLayout);

    // ---------- Live Output panel (right) ----------
    m_slideCounterLabel = new QLabel(this);
    m_slideCounterLabel->setStyleSheet("color: #888; padding: 4px 8px;");

    auto *prevButton = new QToolButton(this);
    prevButton->setText(QStringLiteral("\u25C0"));
    connect(prevButton, &QToolButton::clicked, m_model, &ScheduleModel::retreatLive);

    auto *nextButton = new QToolButton(this);
    nextButton->setText(QStringLiteral("\u25B6"));
    connect(nextButton, &QToolButton::clicked, m_model, &ScheduleModel::advanceLive);

    auto *footerLayout = new QHBoxLayout();
    footerLayout->addWidget(m_slideCounterLabel);
    footerLayout->addStretch(1);
    footerLayout->addWidget(prevButton);
    footerLayout->addWidget(nextButton);

    auto *liveOutputLayout = new QVBoxLayout();
    liveOutputLayout->setContentsMargins(0, 0, 0, 0);
    liveOutputLayout->setSpacing(0);
    liveOutputLayout->addWidget(makePanelHeader(tr("Live Output"), this));
    liveOutputLayout->addWidget(m_liveOutputView, 1);
    liveOutputLayout->addLayout(footerLayout);

    m_liveOutputPanel = new QWidget(this);
    m_liveOutputPanel->setLayout(liveOutputLayout);

    // ---------- Top three-column splitter ----------
    auto *topSplitter = new QSplitter(this);
    topSplitter->addWidget(m_schedulePanel);
    topSplitter->addWidget(m_liveEditorPanel);
    topSplitter->addWidget(m_liveOutputPanel);
    topSplitter->setStretchFactor(0, 2);
    topSplitter->setStretchFactor(1, 3);
    topSplitter->setStretchFactor(2, 3);

    // ---------- Bottom media library ----------
    m_mediaLibrary = new MediaLibraryPanel(this);
    connect(m_mediaLibrary, &MediaLibraryPanel::mediaActivated, this, &OperatorWindow::onMediaActivated);

    // ---------- Overall vertical split ----------
    auto *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(m_mediaLibrary);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    auto *centralLayout = new QVBoxLayout();
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_historyBar);
    centralLayout->addWidget(mainSplitter, 1);

    auto *central = new QWidget(this);
    central->setLayout(centralLayout);
    return central;
}

// ---------------------------------------------------------------------
// Schedule actions
// ---------------------------------------------------------------------

void OperatorWindow::onAddSlideClicked()
{
    bool ok = false;
    const QString text = QInputDialog::getMultiLineText(
        this, tr("Add Slide"), tr("Slide text:"), QString(), &ok);
    if (!ok || text.trimmed().isEmpty())
        return;

    const QString label = text.section('\n', 0, 0).left(40);
    m_model->addSlide(Slide(label, text));
}

void OperatorWindow::onRemoveSlideClicked()
{
    const int row = m_scheduleList->currentRow();
    if (row >= 0)
        m_model->removeSlideAt(row);
}

void OperatorWindow::onScheduleRowChanged(int row)
{
    if (row < 0)
        return;
    m_model->goLiveFromSchedule(row);
}

void OperatorWindow::onMediaActivated(const QString &label, const QColor &background, const QString &imagePath)
{
    // A real imported image supplies its own visual, so no text overlay;
    // the placeholder color swatches show their label as overlay text
    // since they have no actual picture to display.
    Slide slide(label, imagePath.isEmpty() ? label : QString(), background);
    slide.backgroundImagePath = imagePath;
    m_model->sendMediaLive(slide);
}

void OperatorWindow::onEditOptions()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Options"));

    auto *autoAddCheck = new QCheckBox(
        tr("Automatically add media items to the schedule when sent live"), &dialog);
    autoAddCheck->setChecked(m_model->autoAddMediaToSchedule());

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(autoAddCheck);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        m_model->setAutoAddMediaToSchedule(autoAddCheck->isChecked());
    }
}

void OperatorWindow::onScheduleChanged()
{
    rebuildScheduleList();
}

void OperatorWindow::onLiveContentChanged()
{
    refreshScheduleHighlighting();
    refreshLiveOutputFooter();

    if (const Slide *live = m_model->liveSlide()) {
        m_liveEditorHeader->setText(tr("Live - %1").arg(live->label));
    } else {
        m_liveEditorHeader->setText(tr("Live"));
    }
}

void OperatorWindow::rebuildScheduleList()
{
    QSignalBlocker blocker(m_scheduleList);
    m_scheduleList->clear();
    for (int i = 0; i < m_model->count(); ++i) {
        m_scheduleList->addItem(m_model->slideAt(i).label);
    }
    const int rowToSelect = (m_model->liveScheduleIndex() >= 0) ? m_model->liveScheduleIndex() : m_model->scheduleCursor();
    m_scheduleList->setCurrentRow(rowToSelect);
    refreshScheduleHighlighting();
    refreshLiveOutputFooter();
}

void OperatorWindow::refreshScheduleHighlighting()
{
    for (int i = 0; i < m_scheduleList->count(); ++i) {
        QListWidgetItem *item = m_scheduleList->item(i);
        const QString baseLabel = m_model->slideAt(i).label;
        if (i == m_model->liveScheduleIndex() && !m_model->isBlackout()) {
            item->setText(tr("\u25CF ON AIR  %1").arg(baseLabel));
            item->setForeground(QColor("#e74c3c"));
        } else {
            item->setText(baseLabel);
            item->setForeground(QApplication::palette().text().color());
        }
    }
}

void OperatorWindow::refreshLiveOutputFooter()
{
    if (m_model->count() == 0) {
        m_slideCounterLabel->setText(tr("No slides"));
    } else if (m_model->scheduleCursor() < 0) {
        m_slideCounterLabel->setText(tr("Slide - of %1").arg(m_model->count()));
    } else {
        m_slideCounterLabel->setText(tr("Slide %1 of %2")
            .arg(m_model->scheduleCursor() + 1)
            .arg(m_model->count()));
    }
}

// ---------------------------------------------------------------------
// Toolbar / menu action handlers
// ---------------------------------------------------------------------

void OperatorWindow::onNewSchedule()
{
    if (m_model->count() > 0) {
        const auto reply = QMessageBox::question(
            this, tr("New Schedule"),
            tr("Clear the current schedule and start a new one?"),
            QMessageBox::Yes | QMessageBox::Cancel);
        if (reply != QMessageBox::Yes)
            return;
    }
    m_model->clearAll();
}

void OperatorWindow::onGoLiveAction()
{
    showOutputWindow();
    m_actLive->setChecked(true);
}

void OperatorWindow::onBlackToggled(bool checked)
{
    m_model->setBlackout(checked);
}

void OperatorWindow::onClearAction()
{
    // TODO: differentiate from Black -- Clear should eventually remove
    // content while leaving the last background, rather than cutting to
    // solid black. For now both land on the same safe "nothing showing"
    // state.
    m_model->setBlackout(true);
    m_actBlack->setChecked(true);
}

void OperatorWindow::onLiveOutputToggled(bool checked)
{
    if (checked)
        showOutputWindow();
    else
        hideOutputWindow();
}

void OperatorWindow::onAbout()
{
    QMessageBox::about(this, tr("About SanctifyLive"),
        tr("<b>SanctifyLive</b><br>A presentation tool for churches.<br><br>"
           "This build is an early milestone: media/song libraries are placeholders, "
           "and the interface is being iterated on."));
}

void OperatorWindow::onDisplayClientCountChanged(int count)
{
    m_networkStatusLabel->setText(localNetworkStatusText(count));
}

QString OperatorWindow::localNetworkStatusText(int clientCount) const
{
    const QString address = firstLanIPv4Address();
    const QString deviceWord = (clientCount == 1) ? tr("device") : tr("devices");
    return tr("Stage Display: %1:%2  \u2022  %3 %4 connected")
        .arg(address)
        .arg(m_slideServer->port())
        .arg(clientCount)
        .arg(deviceWord);
}

void OperatorWindow::showOutputWindow()
{
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

void OperatorWindow::hideOutputWindow()
{
    m_outputWindow->hide();
}

// ---------------------------------------------------------------------
// Window-level behavior
// ---------------------------------------------------------------------

void OperatorWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    // Whenever the operator window becomes active again, bring the output
    // window back above whatever was covering it. raise() only reorders
    // stacking -- it does not steal keyboard focus.
    if (event->type() == QEvent::ActivationChange && isActiveWindow()) {
        if (m_outputWindow->isVisible())
            m_outputWindow->raise();
    }
}

void OperatorWindow::closeEvent(QCloseEvent *event)
{
    m_outputWindow->close();
    QMainWindow::closeEvent(event);
}

void OperatorWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_Space:
    case Qt::Key_Down:
        m_model->advanceLive();
        break;
    case Qt::Key_Left:
    case Qt::Key_Up:
        m_model->retreatLive();
        break;
    case Qt::Key_B:
        m_actBlack->toggle();
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }
    event->accept();
}