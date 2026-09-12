#include "OperatorWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>

#include "OutputWindow.h"
#include "ScheduleModel.h"

OperatorWindow::OperatorWindow(QWidget *parent)
    : QMainWindow(parent), m_model(new ScheduleModel(this))
{
    setWindowTitle(tr("SanctifyLive - Operator"));
    resize(1100, 650);

    // Two OutputWindow instances share the same model: one is the real
    // congregation-facing output, the other is embedded here as a live
    // preview so the operator can see exactly what's live without a
    // second monitor (matching EasyWorship's "Live Output View").
    m_outputWindow = new OutputWindow(m_model, nullptr);
    m_livePreview = new OutputWindow(m_model, this);
    m_livePreview->setMinimumHeight(160);

    // --- Left: schedule list ---
    m_scheduleList = new QListWidget(this);
    connect(m_scheduleList, &QListWidget::currentRowChanged,
            this, &OperatorWindow::onScheduleItemActivated);

    m_addButton = new QPushButton(tr("+ Add Slide"), this);
    m_removeButton = new QPushButton(tr("Remove Slide"), this);
    connect(m_addButton, &QPushButton::clicked, this, &OperatorWindow::onAddSlideClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &OperatorWindow::onRemoveSlideClicked);

    auto *scheduleButtons = new QHBoxLayout();
    scheduleButtons->addWidget(m_addButton);
    scheduleButtons->addWidget(m_removeButton);

    auto *scheduleLayout = new QVBoxLayout();
    scheduleLayout->addWidget(new QLabel(tr("Schedule"), this));
    scheduleLayout->addWidget(m_scheduleList, /*stretch=*/1);
    scheduleLayout->addLayout(scheduleButtons);

    auto *scheduleBox = new QWidget(this);
    scheduleBox->setLayout(scheduleLayout);

    // --- Center: live preview + next-slide readout ---
    m_nextSlideLabel = new QLabel(tr("Next: -"), this);
    m_nextSlideLabel->setStyleSheet("color: #888; font-style: italic;");

    m_toggleOutputButton = new QPushButton(tr("Show Output Window"), this);
    connect(m_toggleOutputButton, &QPushButton::clicked,
            this, &OperatorWindow::onToggleOutputWindow);

    auto *previewLayout = new QVBoxLayout();
    previewLayout->addWidget(new QLabel(tr("Live"), this));
    previewLayout->addWidget(m_livePreview, /*stretch=*/1);
    previewLayout->addWidget(m_nextSlideLabel);
    previewLayout->addWidget(m_toggleOutputButton);

    auto *previewBox = new QWidget(this);
    previewBox->setLayout(previewLayout);

    // --- Bottom: transport controls (the buttons used every service) ---
    m_prevButton = new QPushButton(tr("<< Previous"), this);
    m_nextButton = new QPushButton(tr("Next >>"), this);
    m_blackButton = new QPushButton(tr("Black / Clear"), this);
    m_blackButton->setCheckable(true);
    m_blackButton->setStyleSheet("QPushButton:checked { background-color: #c0392b; color: white; }");

    connect(m_prevButton, &QPushButton::clicked, m_model, &ScheduleModel::retreat);
    connect(m_nextButton, &QPushButton::clicked, m_model, &ScheduleModel::advance);
    connect(m_blackButton, &QPushButton::toggled, m_model, &ScheduleModel::setBlackout);

    auto *transportLayout = new QHBoxLayout();
    transportLayout->addWidget(m_prevButton);
    transportLayout->addWidget(m_nextButton);
    transportLayout->addStretch(1);
    transportLayout->addWidget(m_blackButton);

    auto *transportBox = new QGroupBox(tr("Live Controls"), this);
    transportBox->setLayout(transportLayout);

    // --- Assemble ---
    auto *splitter = new QSplitter(this);
    splitter->addWidget(scheduleBox);
    splitter->addWidget(previewBox);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    auto *centralLayout = new QVBoxLayout();
    centralLayout->addWidget(splitter, /*stretch=*/1);
    centralLayout->addWidget(transportBox);

    auto *central = new QWidget(this);
    central->setLayout(centralLayout);
    setCentralWidget(central);

    connect(m_model, &ScheduleModel::scheduleChanged, this, &OperatorWindow::onScheduleChanged);
    connect(m_model, &ScheduleModel::liveContentChanged, this, &OperatorWindow::onLiveContentChanged);

    // A couple of starter slides so the app isn't empty on first launch.
    m_model->addSlide(Slide(tr("Welcome"), tr("Welcome to the service"), QColor("#1a1a2e")));
    m_model->addSlide(Slide(tr("Announcement"), tr("Coffee & fellowship after the service"), QColor("#16213e")));

    setFocusPolicy(Qt::StrongFocus);
}

OperatorWindow::~OperatorWindow()
{
    // m_outputWindow has no parent (it's a real top-level window so it can
    // be moved to a second monitor independent of the operator window),
    // so it won't be destroyed automatically by Qt's parent/child cleanup.
    delete m_outputWindow;
}

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

void OperatorWindow::onScheduleItemActivated(int row)
{
    if (row >= 0)
        m_model->goToIndex(row);
}

void OperatorWindow::onScheduleChanged()
{
    rebuildScheduleList();
}

void OperatorWindow::onLiveContentChanged()
{
    // Keep the list selection in sync even when navigation happened via
    // keyboard shortcut rather than a click, so the operator always sees
    // which item is live.
    QSignalBlocker blocker(m_scheduleList);
    m_scheduleList->setCurrentRow(m_model->currentIndex());
    updatePreviewLabels();
}

void OperatorWindow::rebuildScheduleList()
{
    QSignalBlocker blocker(m_scheduleList);
    m_scheduleList->clear();
    for (int i = 0; i < m_model->count(); ++i) {
        m_scheduleList->addItem(m_model->slideAt(i).label);
    }
    m_scheduleList->setCurrentRow(m_model->currentIndex());
    updatePreviewLabels();
}

void OperatorWindow::updatePreviewLabels()
{
    if (const Slide *next = m_model->nextSlide()) {
        m_nextSlideLabel->setText(tr("Next: %1").arg(next->label));
    } else {
        m_nextSlideLabel->setText(tr("Next: (end of schedule)"));
    }
}

void OperatorWindow::onToggleOutputWindow()
{
    if (m_outputWindow->isVisible()) {
        m_outputWindow->hide();
        m_toggleOutputButton->setText(tr("Show Output Window"));
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
    m_toggleOutputButton->setText(tr("Hide Output Window"));
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
        m_blackButton->toggle(); // routes through the button so its checked state stays in sync
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }
    event->accept();
}