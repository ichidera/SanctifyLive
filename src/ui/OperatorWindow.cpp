#include "ui/OperatorWindow.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

#include "core/model/ScheduleIO.h"
#include "core/model/ScheduleModel.h"
#include "core/render/RenderResolution.h"
#include "core/render/SlideRenderer.h"
#include "ui/MediaLibraryPanel.h"
#include "ui/OutputWindow.h"
#include "ui/SlideCanvas.h"
#include "ui/Theme.h"

OperatorWindow::OperatorWindow(QWidget *parent)
    : QMainWindow(parent), m_model(new ScheduleModel(this))
{
    setWindowTitle(tr("SanctifyLive - Operator"));
    resize(1360, 820);

    // Two OutputWindow instances share the same model: one is the real
    // congregation-facing output, the other is embedded here as the
    // "Live" pane so the operator can see exactly what's live without a
    // second monitor. A third, independent SlideCanvas shows whichever
    // Schedule row is merely *selected* -- see the Preview/Live split in
    // buildMainRow().
    m_outputWindow = new OutputWindow(m_model, nullptr);
    m_livePreview = new OutputWindow(m_model, this);
    m_livePreview->setMinimumHeight(220);
    m_previewCanvas = new SlideCanvas(this);
    m_previewCanvas->setMinimumHeight(220);

    buildMenuBar(buildToolBar());

    auto *centralLayout = new QVBoxLayout();
    centralLayout->setContentsMargins(12, 12, 12, 12);
    centralLayout->setSpacing(10);
    centralLayout->addWidget(buildMainRow(), /*stretch=*/3);
    centralLayout->addWidget(buildTransportRow());
    centralLayout->addWidget(buildLowerArea(), /*stretch=*/4);

    auto *central = new QWidget(this);
    central->setLayout(centralLayout);
    setCentralWidget(central);

    statusBar()->setSizeGripEnabled(false);
    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel, /*stretch=*/1);
    auto *wakeButton = new QPushButton(tr("Wake Display"), this);
    wakeButton->setToolTip(tr("Bring the output window to the front of the screen it's on."));
    connect(wakeButton, &QPushButton::clicked, this, &OperatorWindow::onWakeDisplayClicked);
    statusBar()->addPermanentWidget(wakeButton);

    connect(m_model, &ScheduleModel::scheduleChanged, this, &OperatorWindow::onScheduleChanged);
    connect(m_model, &ScheduleModel::liveContentChanged, this, &OperatorWindow::onLiveContentChanged);
    connect(m_model, &ScheduleModel::historyChanged, this, &OperatorWindow::onHistoryChanged);

    // A couple of starter slides so the app isn't empty on first launch.
    m_model->addSlide(Slide(tr("Welcome"), tr("Welcome to the service"), QColor("#1a1a2e")));
    m_model->addSlide(Slide(tr("Announcement"), tr("Coffee & fellowship after the service"), QColor("#16213e")));

    updateLiveIndicator();
    updateStatusBar();
    setFocusPolicy(Qt::StrongFocus);
}

OperatorWindow::~OperatorWindow()
{
    // m_outputWindow has no parent (it's a real top-level window so it can
    // be moved to a second monitor independent of the operator window),
    // so it won't be destroyed automatically by Qt's parent/child cleanup.
    delete m_outputWindow;
}

// ---------------------------------------------------------------------
// Layout construction
// ---------------------------------------------------------------------

QToolBar *OperatorWindow::buildToolBar()
{
    auto *toolBar = addToolBar(tr("Main"));
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(1, 1)); // text-only actions; no bundled icon set yet

    auto addStub = [toolBar](const QString &text, const QString &tooltip) {
        QAction *action = toolBar->addAction(text);
        action->setToolTip(tooltip);
        action->setEnabled(false);
        return action;
    };

    connect(toolBar->addAction(tr("New")), &QAction::triggered, this, &OperatorWindow::onNewSchedule);
    connect(toolBar->addAction(tr("Open")), &QAction::triggered, this, &OperatorWindow::onOpenSchedule);
    connect(toolBar->addAction(tr("Save")), &QAction::triggered, this, &OperatorWindow::onSaveSchedule);
    addStub(tr("Web"), tr("Web publishing -- not implemented yet."));
    addStub(tr("Remote"), tr("Remote control from a phone/tablet -- not implemented yet."));

    toolBar->addSeparator();

    auto *spacer = new QWidget(toolBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);

    m_goLiveButton = new QPushButton(tr("Go Live"), toolBar);
    m_goLiveButton->setObjectName("goLiveButton");
    m_goLiveButton->setCheckable(true);
    connect(m_goLiveButton, &QPushButton::toggled, this, &OperatorWindow::onToggleOutputWindow);
    toolBar->addWidget(m_goLiveButton);

    toolBar->addSeparator();

    addStub(tr("Alerts"), tr("On-screen alerts/announcements -- not implemented yet."));
    addStub(tr("Logo"), tr("Logo/branding slide -- not implemented yet."));

    m_blackButton = new QToolButton(toolBar);
    m_blackButton->setObjectName("blackButton");
    m_blackButton->setText(tr("Black"));
    m_blackButton->setCheckable(true);
    m_blackButton->setToolTip(tr("Cut the live output to black (shortcut: B)"));
    connect(m_blackButton, &QToolButton::toggled, m_model, &ScheduleModel::setBlackout);
    toolBar->addWidget(m_blackButton);

    m_clearButton = new QToolButton(toolBar);
    m_clearButton->setObjectName("clearButton");
    m_clearButton->setText(tr("Clear"));
    m_clearButton->setToolTip(tr("Immediately cut live output to black"));
    connect(m_clearButton, &QToolButton::clicked, this, &OperatorWindow::onClearClicked);
    toolBar->addWidget(m_clearButton);

    m_liveIndicator = new QLabel(toolBar);
    m_liveIndicator->setObjectName("liveTitle");
    m_liveIndicator->setContentsMargins(8, 0, 4, 0);
    toolBar->addWidget(m_liveIndicator);

    return toolBar;
}

void OperatorWindow::buildMenuBar(QToolBar *toolBar)
{
    // The main menu bar (File/Edit/Live/Profiles/View/Help) sits above
    // the toolbar -- QMainWindow places menuBar() there automatically.
    // Every entry here is just a second way to reach a command the
    // toolbar/Schedule panel already expose; nothing new is implemented
    // just for the menu.
    QMenuBar *bar = menuBar();
    bar->setObjectName("mainMenuBar");

    QMenu *fileMenu = bar->addMenu(tr("&File"));
    connect(fileMenu->addAction(tr("&New Schedule")), &QAction::triggered,
            this, &OperatorWindow::onNewSchedule);
    connect(fileMenu->addAction(tr("&Open Schedule\u2026")), &QAction::triggered,
            this, &OperatorWindow::onOpenSchedule);
    connect(fileMenu->addAction(tr("&Save Schedule\u2026")), &QAction::triggered,
            this, &OperatorWindow::onSaveSchedule);
    fileMenu->addSeparator();
    connect(fileMenu->addAction(tr("E&xit")), &QAction::triggered, this, &QWidget::close);

    QMenu *editMenu = bar->addMenu(tr("&Edit"));
    connect(editMenu->addAction(tr("&Add Slide\u2026")), &QAction::triggered,
            this, &OperatorWindow::onAddSlideClicked);
    connect(editMenu->addAction(tr("&Remove Selected Slide")), &QAction::triggered,
            this, &OperatorWindow::onRemoveSlideClicked);

    QMenu *liveMenu = bar->addMenu(tr("&Live"));
    QAction *goLiveAction = liveMenu->addAction(tr("&Go Live"));
    goLiveAction->setCheckable(true);
    // Two-way sync so the menu checkmark and the toolbar button never
    // disagree about whether output is live, regardless of which one
    // the operator actually clicks.
    connect(goLiveAction, &QAction::toggled, m_goLiveButton, &QPushButton::setChecked);
    connect(m_goLiveButton, &QPushButton::toggled, goLiveAction, &QAction::setChecked);

    QAction *blackAction = liveMenu->addAction(tr("&Black"));
    blackAction->setCheckable(true);
    connect(blackAction, &QAction::toggled, m_blackButton, &QToolButton::setChecked);
    connect(m_blackButton, &QToolButton::toggled, blackAction, &QAction::setChecked);

    connect(liveMenu->addAction(tr("&Clear")), &QAction::triggered,
            this, &OperatorWindow::onClearClicked);
    liveMenu->addSeparator();
    connect(liveMenu->addAction(tr("Ne&xt")), &QAction::triggered, m_model, &ScheduleModel::advance);
    connect(liveMenu->addAction(tr("Pre&vious")), &QAction::triggered, m_model, &ScheduleModel::retreat);

    // Profiles (per-venue/per-service settings presets) has no backing
    // feature yet -- see README roadmap -- so this menu is a labeled,
    // disabled placeholder rather than something that pretends to work.
    QMenu *profilesMenu = bar->addMenu(tr("&Profiles"));
    QAction *manageProfiles = profilesMenu->addAction(tr("&Manage Profiles\u2026"));
    manageProfiles->setEnabled(false);
    manageProfiles->setToolTip(tr("Profiles aren't implemented yet -- see the roadmap in README.md."));

    QMenu *viewMenu = bar->addMenu(tr("&View"));
    connect(viewMenu->addAction(tr("&Wake Display")), &QAction::triggered,
            this, &OperatorWindow::onWakeDisplayClicked);
    QAction *toggleToolbar = viewMenu->addAction(tr("Show &Toolbar"));
    toggleToolbar->setCheckable(true);
    toggleToolbar->setChecked(true);
    connect(toggleToolbar, &QAction::toggled, toolBar, &QToolBar::setVisible);

    QMenu *helpMenu = bar->addMenu(tr("&Help"));
    connect(helpMenu->addAction(tr("&About SanctifyLive")), &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About SanctifyLive"),
                            tr("SanctifyLive\n\nA church presentation program.\n"
                               "See README.md for features and the roadmap."));
    });
}

QFrame *OperatorWindow::wrapInPanelCard(const QString &title, QWidget *content, const char *titleObjectName)
{
    auto *card = new QFrame(this);
    card->setObjectName("panelCard");

    auto *label = new QLabel(title, card);
    label->setObjectName(titleObjectName);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    layout->addWidget(label);
    layout->addWidget(content, /*stretch=*/1);

    return card;
}

QWidget *OperatorWindow::buildMainRow()
{
    // Schedule | Preview | Live, side by side -- the same three-panel
    // arrangement OpenLP uses for its Service Manager + Preview/Live
    // slide controllers, rather than tucking the run order away in the
    // bottom strip. Schedule sits to the left of Preview so the operator
    // reads left-to-right: "what's coming up" -> "what's staged" ->
    // "what's actually live."
    auto *splitter = new QSplitter(this);
    splitter->addWidget(wrapInPanelCard(tr("Schedule"), buildSchedulePanel()));
    splitter->addWidget(wrapInPanelCard(tr("Preview"), m_previewCanvas));
    splitter->addWidget(wrapInPanelCard(tr("Live"), m_livePreview, "liveTitle"));
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);
    return splitter;
}

QWidget *OperatorWindow::buildTransportRow()
{
    m_prevButton = new QPushButton(tr("\u2039\u2039 Previous"), this);
    m_nextButton = new QPushButton(tr("Next \u203A\u203A"), this);
    connect(m_prevButton, &QPushButton::clicked, m_model, &ScheduleModel::retreat);
    connect(m_nextButton, &QPushButton::clicked, m_model, &ScheduleModel::advance);

    m_nextSlideLabel = new QLabel(tr("Next: -"), this);
    m_nextSlideLabel->setObjectName("nextSlideLabel");
    m_nextSlideLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *row = new QWidget(this);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_prevButton);
    layout->addWidget(m_nextButton);
    layout->addStretch(1);
    layout->addWidget(m_nextSlideLabel);
    return row;
}

QWidget *OperatorWindow::buildLowerArea()
{
    // Right-hand column, stacked top-to-bottom: ITEM PREVIEW, HISTORY,
    // TRANSCRIPTION. Item Preview lives here -- next to History -- and
    // deliberately NOT wedged inside the Media tab (see MediaLibraryPanel's
    // own glossary note): it previews whatever's selected across Content
    // Tabs in general, not just Media, so it doesn't belong to any one
    // tab. This is a different "preview" than the Main Row's Preview
    // panel: that one stages the next *service* item; this one previews
    // *library content* before it's even added to Schedule.
    auto *rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(wrapInPanelCard(tr("Item Preview"), buildItemPreviewPanel()));
    rightSplitter->addWidget(wrapInPanelCard(tr("History"), buildHistoryPanel()));
    rightSplitter->addWidget(wrapInPanelCard(tr("Transcription"), buildTranscriptionPanel()));
    rightSplitter->setStretchFactor(0, 2);
    rightSplitter->setStretchFactor(1, 1);
    rightSplitter->setStretchFactor(2, 1);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(buildContentTabs());
    splitter->addWidget(rightSplitter);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    return splitter;
}

QTabWidget *OperatorWindow::buildContentTabs()
{
    auto makePlaceholder = [this](const QString &text) {
        auto *label = new QLabel(text, this);
        label->setObjectName("nextSlideLabel");
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        auto *wrapper = new QWidget(this);
        auto *layout = new QVBoxLayout(wrapper);
        layout->addStretch(1);
        layout->addWidget(label);
        layout->addStretch(1);
        return wrapper;
    };

    auto *tabs = new QTabWidget(this);
    tabs->addTab(makePlaceholder(
                     tr("No song library yet.\nUse \u201c+ Add Slide\u201d in the Schedule panel, "
                        "or double-click a background in the Media tab.")),
                 tr("Songs"));
    tabs->addTab(makePlaceholder(
                     tr("Scripture lookup isn't implemented yet.\nSee the roadmap in README.md.")),
                 tr("Scriptures"));

    m_mediaPanel = new MediaLibraryPanel(this);
    connect(m_mediaPanel, &MediaLibraryPanel::mediaActivated, this, &OperatorWindow::onMediaActivated);
    connect(m_mediaPanel, &MediaLibraryPanel::previewRequested, this, &OperatorWindow::onMediaPreviewRequested);
    tabs->addTab(m_mediaPanel, tr("Media"));

    tabs->addTab(makePlaceholder(tr("Presentations aren't implemented yet.\nSee the roadmap in README.md.")),
                 tr("Presentations"));
    tabs->addTab(makePlaceholder(tr("Themes aren't implemented yet.\nSee the roadmap in README.md.")),
                 tr("Themes"));

    tabs->setCurrentWidget(m_mediaPanel);
    return tabs;
}

QWidget *OperatorWindow::buildSchedulePanel()
{
    // Single click stages a row in Preview without going live; double-
    // click (or Next/Previous/keyboard shortcuts) commits it live. This
    // matches OpenLP's Service Manager, where a single click on a
    // service item sends it to the Preview slide controller and a
    // double-click sends it straight to Live.
    m_scheduleList = new QListWidget(this);
    m_scheduleList->setWordWrap(true);
    connect(m_scheduleList, &QListWidget::currentRowChanged, this, &OperatorWindow::onScheduleItemSelected);
    connect(m_scheduleList, &QListWidget::itemActivated, this, &OperatorWindow::onScheduleItemActivated);

    m_addButton = new QPushButton(tr("+ Add Slide"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    connect(m_addButton, &QPushButton::clicked, this, &OperatorWindow::onAddSlideClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &OperatorWindow::onRemoveSlideClicked);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_removeButton);

    auto *hint = new QLabel(tr("Click to preview, double-click (or press Go Live) to put a slide on air."), this);
    hint->setObjectName("nextSlideLabel");
    hint->setWordWrap(true);

    auto *wrapper = new QWidget(this);
    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scheduleList, /*stretch=*/1);
    layout->addWidget(hint);
    layout->addLayout(buttonRow);
    return wrapper;
}

QWidget *OperatorWindow::buildItemPreviewPanel()
{
    // Shows a pixel-faithful render of whatever's currently
    // selected/hovered in Content Tabs (Media today; Songs/
    // Presentations/Themes once they're real), using the exact same
    // SlideRenderer/SlideCanvas pipeline as Schedule/Preview/Live, so
    // what's shown here never diverges from what actually appears once
    // it's added to Schedule and put on air.
    m_itemPreviewCanvas = new SlideCanvas(this);
    m_itemPreviewCanvas->setMinimumHeight(120);
    m_itemPreviewCanvas->setPixmap(SlideRenderer::render(nullptr, kDesignResolution));

    m_itemPreviewLabel = new QLabel(tr("Select an item to preview"), this);
    m_itemPreviewLabel->setObjectName("nextSlideLabel");
    m_itemPreviewLabel->setAlignment(Qt::AlignCenter);
    m_itemPreviewLabel->setWordWrap(true);

    auto *wrapper = new QWidget(this);
    auto *layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(m_itemPreviewCanvas, /*stretch=*/1);
    layout->addWidget(m_itemPreviewLabel);
    return wrapper;
}

QWidget *OperatorWindow::buildHistoryPanel()
{
    // An append-only, read-only log of what has actually gone live, most
    // recent first -- distinct from the Schedule list, which is the plan
    // rather than the record. Nothing here can be clicked to change
    // what's live; it exists purely so an operator can answer "what did
    // we just show" without scrolling back through the Schedule.
    m_historyList = new QListWidget(this);
    m_historyList->setWordWrap(true);
    m_historyList->setSelectionMode(QAbstractItemView::NoSelection);
    m_historyList->setFocusPolicy(Qt::NoFocus);
    rebuildHistoryList();
    return m_historyList;
}

QWidget *OperatorWindow::buildTranscriptionPanel()
{
    // Live speech-to-text isn't implemented yet -- there's no audio
    // capture/transcription pipeline in the project at all (see README
    // roadmap). This is an honest, disabled stub rather than a mock that
    // pretends to transcribe, matching how Songs/Scriptures/Presentations/
    // Themes are handled elsewhere in this window.
    auto *label = new QLabel(
        tr("Live transcription isn't implemented yet.\nSee the roadmap in README.md."), this);
    label->setObjectName("nextSlideLabel");
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);

    auto *startButton = new QPushButton(tr("Start Transcription"), this);
    startButton->setEnabled(false);
    startButton->setToolTip(tr("Live transcription isn't implemented yet -- see the roadmap in README.md."));

    auto *wrapper = new QWidget(this);
    auto *layout = new QVBoxLayout(wrapper);
    layout->addStretch(1);
    layout->addWidget(label);
    layout->addWidget(startButton, 0, Qt::AlignCenter);
    layout->addStretch(1);
    return wrapper;
}

// ---------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------

void OperatorWindow::onAddSlideClicked()
{
    bool ok = false;
    const QString text = QInputDialog::getMultiLineText(
        this, tr("Add Slide"), tr("Slide text:"), QString(), &ok);
    if (!ok || text.trimmed().isEmpty())
        return;

    // Use the first line as the schedule-list label so the operator can
    // scan the list without the full text cluttering it.
    const QString label = text.section('\n', 0, 0).left(40);
    m_model->addSlide(Slide(label, text));
}

void OperatorWindow::onRemoveSlideClicked()
{
    const int row = m_scheduleList->currentRow();
    if (row >= 0)
        m_model->removeSlideAt(row);
}

void OperatorWindow::onScheduleItemSelected(int row)
{
    // Selecting a Schedule row only updates the Preview pane -- it does
    // NOT go live. That mirrors OpenLP's Service Manager (single click
    // -> Preview slide controller) and lets an operator stage the next
    // item before cutting to it.
    updatePreviewForRow(row);
}

void OperatorWindow::onScheduleItemActivated(QListWidgetItem *item)
{
    const int row = m_scheduleList->row(item);
    if (row >= 0)
        m_model->goToIndex(row);
}

void OperatorWindow::onScheduleChanged()
{
    rebuildScheduleList();
}

void OperatorWindow::onHistoryChanged()
{
    rebuildHistoryList();
}

void OperatorWindow::onLiveContentChanged()
{
    // Keep the list selection in sync even when navigation happened via
    // keyboard shortcut or the transport buttons, so the operator always
    // sees which item is live. This also drives the Preview pane back to
    // the live row whenever the live position itself moves, so Preview
    // and Live agree unless the operator has deliberately staged
    // something else.
    QSignalBlocker blocker(m_scheduleList);
    m_scheduleList->setCurrentRow(m_model->currentIndex());
    updatePreviewForRow(m_model->currentIndex());
    updateNextSlideLabel();
    if (m_blackButton->isChecked() != m_model->isBlackout()) {
        QSignalBlocker blackBlocker(m_blackButton);
        m_blackButton->setChecked(m_model->isBlackout());
    }
}

void OperatorWindow::rebuildScheduleList()
{
    QSignalBlocker blocker(m_scheduleList);
    m_scheduleList->clear();
    for (int i = 0; i < m_model->count(); ++i) {
        const Slide &slide = m_model->slideAt(i);
        const QString preview = QString("%1\n%2")
                                     .arg(slide.label)
                                     .arg(slide.text.left(60).replace('\n', ' '));
        m_scheduleList->addItem(preview);
    }
    m_scheduleList->setCurrentRow(m_model->currentIndex());
    updateNextSlideLabel();
    updateStatusBar();
}

void OperatorWindow::rebuildHistoryList()
{
    // Most-recent-first so the operator doesn't have to scroll to see
    // what just went live.
    m_historyList->clear();
    const QVector<HistoryEntry> &history = m_model->history();
    for (int i = history.size() - 1; i >= 0; --i) {
        const HistoryEntry &entry = history.at(i);
        const QString row = QString("%1\n%2")
                                 .arg(entry.timestamp.toString("hh:mm:ss"))
                                 .arg(entry.label);
        m_historyList->addItem(row);
    }
}

void OperatorWindow::updatePreviewForRow(int row)
{
    if (row < 0 || row >= m_model->count()) {
        m_previewCanvas->setPixmap(SlideRenderer::render(nullptr, kDesignResolution));
        return;
    }
    m_previewCanvas->setPixmap(SlideRenderer::render(&m_model->slideAt(row), kDesignResolution));
}

void OperatorWindow::updateNextSlideLabel()
{
    if (const Slide *next = m_model->nextSlide()) {
        m_nextSlideLabel->setText(tr("Next: %1").arg(next->label));
    } else {
        m_nextSlideLabel->setText(tr("Next: (end of schedule)"));
    }
}

void OperatorWindow::updateLiveIndicator()
{
    if (m_outputWindow->isVisible()) {
        m_liveIndicator->setText(tr("\u25CF LIVE"));
        m_liveIndicator->setStyleSheet(QString("color: %1; font-weight: 700;").arg(Theme::kAccentRed));
        m_goLiveButton->setText(tr("End Live"));
    } else {
        m_liveIndicator->setText(tr("\u25CB Offline"));
        m_liveIndicator->setStyleSheet(QString("color: %1;").arg(Theme::kTextMuted));
        m_goLiveButton->setText(tr("Go Live"));
    }
}

void OperatorWindow::updateStatusBar()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    const QString outputState = m_outputWindow->isVisible()
        ? (screens.size() > 1 ? tr("Output: fullscreen on secondary display") : tr("Output: windowed (single display)"))
        : tr("Output: hidden");
    m_statusLabel->setText(tr("%1  \u2022  %2 slide(s) in schedule").arg(outputState).arg(m_model->count()));
}

void OperatorWindow::onToggleOutputWindow(bool checked)
{
    if (!checked) {
        m_outputWindow->hide();
        updateLiveIndicator();
        updateStatusBar();
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
    updateLiveIndicator();
    updateStatusBar();
}

void OperatorWindow::onWakeDisplayClicked()
{
    if (m_outputWindow->isVisible()) {
        m_outputWindow->raise();
        m_outputWindow->activateWindow();
    } else {
        m_goLiveButton->setChecked(true); // routes through the same toggle logic
    }
}

void OperatorWindow::onClearClicked()
{
    m_model->setBlackout(true);
}

void OperatorWindow::onNewSchedule()
{
    if (m_model->count() > 0) {
        const auto answer = QMessageBox::question(
            this, tr("New Schedule"),
            tr("Discard the current schedule and start a new, empty one?"));
        if (answer != QMessageBox::Yes)
            return;
    }
    m_model->setSlides({});
}

void OperatorWindow::onOpenSchedule()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Schedule"), QString(), tr("SanctifyLive Schedule (*.json)"));
    if (path.isEmpty())
        return;

    QVector<Slide> slides;
    QString error;
    if (!ScheduleIO::loadFromFile(path, &slides, &error)) {
        QMessageBox::warning(this, tr("Open Schedule"), tr("Couldn't open that file:\n%1").arg(error));
        return;
    }
    m_model->setSlides(slides);
}

void OperatorWindow::onSaveSchedule()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Schedule"), tr("schedule.json"), tr("SanctifyLive Schedule (*.json)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!ScheduleIO::saveToFile(m_model->slides(), path, &error)) {
        QMessageBox::warning(this, tr("Save Schedule"), tr("Couldn't save that file:\n%1").arg(error));
    }
}

void OperatorWindow::onMediaActivated(const QString &name, const QColor &color)
{
    // A modest, real bridge from the (currently cosmetic) Media browser
    // into the live schedule: drop a new slide using this swatch as its
    // background straight onto the end of the Schedule.
    m_model->addSlide(Slide(name, QString(), color));
}

void OperatorWindow::onMediaPreviewRequested(const QString &name, const QColor &color)
{
    // Renders exactly what onMediaActivated() would actually add (same
    // empty-text-body + swatch-background Slide), so the Item Preview
    // panel is never a lie about what double-clicking would produce.
    const Slide previewSlide(name, QString(), color);
    m_itemPreviewCanvas->setPixmap(SlideRenderer::render(&previewSlide, kDesignResolution));
    m_itemPreviewLabel->setText(name);
}

// ---------------------------------------------------------------------
// Window-level events
// ---------------------------------------------------------------------

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
        m_blackButton->toggle(); // routes through the button so its checked state stays in sync
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }
    event->accept();
}
