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

#include "history/HistoryBar.h"
#include "common/IconFactory.h"
#include "media/MediaLibraryPanel.h"
#include "output/OutputWindow.h"
#include "schedule/ScheduleModel.h"
#include "scripture/ScriptureFormatting.h"
#include "settings/SettingsWindow.h"
#include "remote/SlideServer.h"

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
// app -- see src/Android/PROTOCOL.md. Not authoritative: a machine with
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

    m_outputWindow = new OutputWindow(m_model, OutputWindow::Source::Live);
    // Owning it via setParent(this, Qt::Window) rather than leaving it
    // fully parentless is what makes this match EasyWorship's behavior:
    // ONE entry in the taskbar/Alt+Tab, even though the congregation
    // output is, under the hood, a second, independently-positioned
    // window (it still needs to be a real top-level window -- Qt::Window
    // -- so it can be dragged to a second monitor and shown fullscreen
    // there; only the *owner* changes here, not its window-ness).
    m_outputWindow->setParent(this, Qt::Window);
    m_liveEditorView = new OutputWindow(m_model, OutputWindow::Source::Preview, this);
    m_liveOutputView = new OutputWindow(m_model, OutputWindow::Source::Live, this);
    // Give all three the same default profile MediaLibraryPanel's
    // previews start with (see buildWorkspace()) so the real output and
    // every in-app preview agree even before Options has been opened.
    m_outputWindow->setProfile(m_mainOutputProfile);
    m_liveEditorView->setProfile(m_mainOutputProfile);
    m_liveOutputView->setProfile(m_mainOutputProfile);

    buildMenuBar();
    buildToolBar();

    m_historyBar = new HistoryBar(m_model, this);
    setCentralWidget(buildWorkspace());

    m_slideServer = new SlideServer(m_model, this);
    connect(m_slideServer, &SlideServer::clientCountChanged,
            this, &OperatorWindow::onDisplayClientCountChanged);

    m_networkStatusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_networkStatusLabel);

    // Lets the operator force a connected tablet's screen back on without
    // walking over to tap it -- see SlideServer::wakeAll(). Disabled
    // whenever no device is connected, since there's nothing to wake.
    m_wakeDisplayButton = new QPushButton(tr("Wake Display"), this);
    m_wakeDisplayButton->setToolTip(
        tr("Force the connected display's screen on, even if it's locked or asleep"));
    m_wakeDisplayButton->setEnabled(false);
    connect(m_wakeDisplayButton, &QPushButton::clicked,
            this, &OperatorWindow::onWakeDisplayClicked);
    statusBar()->addPermanentWidget(m_wakeDisplayButton);

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
    // m_outputWindow is now parented to `this` (see the constructor's
    // setParent(this, Qt::Window) call), so Qt's normal parent/child
    // cleanup destroys it automatically -- no manual delete needed here
    // anymore, and doing one would double-free it.
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

    liveMenu->addSeparator();
    QAction *actSendLiveToPhone = liveMenu->addAction(tr("Send Live to Phone Only"));
    actSendLiveToPhone->setToolTip(
        tr("Pin whatever is live right now to connected phones, even after the main display moves on"));
    connect(actSendLiveToPhone, &QAction::triggered, this, &OperatorWindow::onSendLiveToPhoneOnly);
    QAction *actMirrorPhone = liveMenu->addAction(tr("Mirror Main Display on Phone"));
    actMirrorPhone->setToolTip(tr("Stop overriding phone content -- go back to always matching the main display"));
    connect(actMirrorPhone, &QAction::triggered, this, &OperatorWindow::onMirrorPhoneToMain);

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
    connect(m_mediaLibrary, &MediaLibraryPanel::mediaSentToPhone, this, &OperatorWindow::onMediaSentToPhone);
    connect(m_mediaLibrary, &MediaLibraryPanel::scriptureActivated, this, &OperatorWindow::onScriptureActivated);
    connect(m_mediaLibrary, &MediaLibraryPanel::songSlideActivated, this, &OperatorWindow::onSongSlideActivated);
    // Themes send live exactly like Media (a full-screen background) --
    // reuse the same slot rather than duplicating it.
    connect(m_mediaLibrary, &MediaLibraryPanel::themeActivated, this, &OperatorWindow::onMediaActivated);
    m_mediaLibrary->setOutputProfile(m_mainOutputProfile);

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

void OperatorWindow::onMediaActivated(const QString &label, const QColor &background, const QString &imagePath,
                                      const QPointF &focus)
{
    // Same construction the Media/Themes preview uses (Slide::fromMediaEntry,
    // src/core/schedule/Slide.h) so what actually goes live is guaranteed
    // to match what the operator just previewed.
    m_model->sendMediaLive(Slide::fromMediaEntry(label, background, imagePath, focus));
}

void OperatorWindow::onMediaSentToPhone(const QString &label, const QColor &background, const QString &imagePath,
                                        const QPointF &focus)
{
    m_model->sendPhoneOverride(Slide::fromMediaEntry(label, background, imagePath, focus));
}

void OperatorWindow::onSendLiveToPhoneOnly()
{
    if (const Slide *live = m_model->liveSlide())
        m_model->sendPhoneOverride(*live);
}

void OperatorWindow::onMirrorPhoneToMain()
{
    m_model->clearPhoneOverride();
}

void OperatorWindow::onScriptureActivated(const QString &reference, const QString &text,
                                           const QString &translationCode)
{
    // The reference (e.g. "John 3:16") becomes the slide's operator-only
    // label; the verse text itself -- annotated with the translation
    // code, the way most projection software footnotes Scripture -- is
    // what's actually projected. Same composeProjectedScripture() call
    // ScripturePanel's own preview makes (see ScripturePanel::updatePreview
    // and src/core/scripture/ScriptureFormatting.h), so "Send Selected"
    // can never put something on screen the preview didn't already show.
    const QString projected = composeProjectedScripture(text, reference, translationCode);
    m_model->sendMediaLive(Slide(reference, projected, Qt::black));
}

void OperatorWindow::onSongSlideActivated(const QString &songTitle, const QString &slideText)
{
    // slideText already has title-prefixing/line-splitting applied by
    // SongPanel::composeSlides() per the current profile's Song settings
    // -- see SongPanel.cpp -- so it's projected as-is, same as the
    // Songs tab's own preview showed.
    m_model->sendMediaLive(Slide(songTitle, slideText, Qt::black));
}

void OperatorWindow::onEditOptions()
{
    if (!m_settingsWindow) {
        m_settingsWindow = new SettingsWindow(m_model, m_slideServer, this);
        connect(m_settingsWindow, &SettingsWindow::mainOutputChanged,
                this, &OperatorWindow::onMainOutputConfigured);
        connect(m_settingsWindow, &SettingsWindow::mainOutputProfileChanged,
                this, &OperatorWindow::onMainOutputProfileChanged);
    }

    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

void OperatorWindow::onMainOutputConfigured(int monitorIndex, const QRect &position)
{
    m_mainOutputConfigured = true;
    m_mainOutputMonitorIndex = monitorIndex;
    m_mainOutputPosition = position;

    // If the congregation-facing window is already up, re-apply its
    // geometry immediately rather than waiting for the next Go Live --
    // the operator just told us where it belongs.
    if (m_outputWindow->isVisible())
        showOutputWindow();
}

void OperatorWindow::onMainOutputProfileChanged(const OutputProfile &profile)
{
    m_mainOutputProfile = profile;

    // The real congregation-facing window and both its in-app mirrors
    // all need the new margins/font/resolution immediately, not just on
    // the next slide change.
    m_outputWindow->setProfile(profile);
    m_liveEditorView->setProfile(profile);
    m_liveOutputView->setProfile(profile);

    // ...and every preview in the bottom resource library (Media's own,
    // plus Scriptures/Songs/Themes) needs to stay in lock-step with what
    // the real output now looks like.
    m_mediaLibrary->setOutputProfile(profile);
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
    m_wakeDisplayButton->setEnabled(count > 0);
}

void OperatorWindow::onWakeDisplayClicked()
{
    m_slideServer->wakeAll();
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

    // Once the operator has actually opened Options and clicked OK on a
    // Main Output monitor choice, that choice wins -- even if it's
    // "None" (monitorIndex < 0), which falls back to a plain windowed
    // view rather than us guessing a monitor they didn't ask for.
    if (m_mainOutputConfigured) {
        if (m_mainOutputMonitorIndex >= 0 && m_mainOutputMonitorIndex < screens.size()) {
            m_outputWindow->setGeometry(screens.at(m_mainOutputMonitorIndex)->geometry());
            m_outputWindow->showFullScreen();
        } else {
            m_outputWindow->resize(960, 540);
            m_outputWindow->show();
        }
        return;
    }

    // Options has never been opened this session -- keep the original
    // best-effort default of preferring a second monitor when one exists.
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