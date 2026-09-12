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
#include <QTabWidget>

#include "OutputWindow.h"
#include "ScheduleModel.h"

namespace {
// A small helper so panel headers ("PREVIEW" / "LIVE") get a consistent
// look without repeating stylesheet strings at every call site.
QLabel *makeHeaderLabel(const QString &text, const QString &accentColor, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral(
        "font-weight: 600; font-size: 12px; letter-spacing: 1px; padding: 4px; "
        "color: %1; background-color: #1a1a1a; border-radius: 3px;").arg(accentColor));
    return label;
}
}

OperatorWindow::OperatorWindow(QWidget *parent)
    : QMainWindow(parent), m_model(new ScheduleModel(this))
{
    setWindowTitle(tr("SanctifyLive - Operator"));
    resize(1280, 720);

    // Three OutputWindow instances share the same model: the real
    // congregation-facing output, an in-app Live mirror (no second
    // monitor required), and a Preview pane the operator can look ahead
    // in without it ever reaching the audience.
    m_outputWindow = new OutputWindow(m_model, OutputWindow::Source::Live, nullptr);
    m_livePreview = new OutputWindow(m_model, OutputWindow::Source::Live, this);
    m_previewPane = new OutputWindow(m_model, OutputWindow::Source::Preview, this);
    m_livePreview->setMinimumHeight(140);
    m_previewPane->setMinimumHeight(140);

    // ---------- Resource tabs (left) ----------
    m_scheduleList = new QListWidget(this);
    m_scheduleList->setAlternatingRowColors(true);
    connect(m_scheduleList, &QListWidget::currentRowChanged,
            this, &OperatorWindow::onScheduleRowChanged);
    connect(m_scheduleList, &QListWidget::itemDoubleClicked,
            this, &OperatorWindow::onScheduleItemDoubleClicked);

    m_addButton = new QPushButton(tr("+ Add Slide"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    connect(m_addButton, &QPushButton::clicked, this, &OperatorWindow::onAddSlideClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &OperatorWindow::onRemoveSlideClicked);

    auto *scheduleButtons = new QHBoxLayout();
    scheduleButtons->addWidget(m_addButton);
    scheduleButtons->addWidget(m_removeButton);

    auto *scheduleTabLayout = new QVBoxLayout();
    scheduleTabLayout->addWidget(m_scheduleList, /*stretch=*/1);
    scheduleTabLayout->addLayout(scheduleButtons);

    auto *scheduleTab = new QWidget(this);
    scheduleTab->setLayout(scheduleTabLayout);

    m_resourceTabs = new QTabWidget(this);
    m_resourceTabs->addTab(scheduleTab, tr("Schedule"));
    // Placeholders: the tabs exist now so the panel structure doesn't
    // need reworking once Songs/Media/Scripture libraries are built.
    m_resourceTabs->addTab(new QWidget(this), tr("Songs"));
    m_resourceTabs->addTab(new QWidget(this), tr("Media"));
    m_resourceTabs->addTab(new QWidget(this), tr("Scripture"));
    m_resourceTabs->setTabEnabled(1, false);
    m_resourceTabs->setTabEnabled(2, false);
    m_resourceTabs->setTabEnabled(3, false);
    m_resourceTabs->setMinimumWidth(300);

    // ---------- Preview pane (center) ----------
    m_goLiveButton = new QPushButton(tr("GO LIVE  \u25B6"), this);
    m_goLiveButton->setObjectName("goLiveButton");
    m_goLiveButton->setMinimumHeight(48);
    connect(m_goLiveButton, &QPushButton::clicked, this, &OperatorWindow::onGoLiveClicked);

    auto *previewLayout = new QVBoxLayout();
    previewLayout->addWidget(makeHeaderLabel(tr("PREVIEW"), "#5dade2", this));
    previewLayout->addWidget(m_previewPane, /*stretch=*/1);
    previewLayout->addWidget(m_goLiveButton);

    auto *previewBox = new QWidget(this);
    previewBox->setLayout(previewLayout);

    // ---------- Live pane (right) ----------
    m_toggleOutputButton = new QPushButton(tr("Show Output Window"), this);
    connect(m_toggleOutputButton, &QPushButton::clicked,
            this, &OperatorWindow::onToggleOutputWindow);

    auto *liveLayout = new QVBoxLayout();
    liveLayout->addWidget(makeHeaderLabel(tr("LIVE"), "#e74c3c", this));
    liveLayout->addWidget(m_livePreview, /*stretch=*/1);
    liveLayout->addWidget(m_toggleOutputButton);

    auto *liveBox = new QWidget(this);
    liveBox->setLayout(liveLayout);

    // ---------- Bottom transport bar ----------
    m_prevButton = new QPushButton(tr("\u25C0\u25C0 Previous"), this);
    m_nextButton = new QPushButton(tr("Next \u25B6\u25B6"), this);
    m_blackButton = new QPushButton(tr("Black / Clear"), this);
    m_blackButton->setObjectName("blackButton");
    m_blackButton->setCheckable(true);

    connect(m_prevButton, &QPushButton::clicked, m_model, &ScheduleModel::retreatLive);
    connect(m_nextButton, &QPushButton::clicked, m_model, &ScheduleModel::advanceLive);
    connect(m_blackButton, &QPushButton::toggled, m_model, &ScheduleModel::setBlackout);

    auto *transportLayout = new QHBoxLayout();
    transportLayout->setSpacing(12);
    transportLayout->addWidget(m_prevButton);
    transportLayout->addWidget(m_nextButton);
    transportLayout->addStretch(1);
    transportLayout->addWidget(m_blackButton);

    auto *transportBox = new QWidget(this);
    transportBox->setObjectName("transportBar");
    transportBox->setLayout(transportLayout);

    // ---------- Assemble ----------
    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_resourceTabs);
    splitter->addWidget(previewBox);
    splitter->addWidget(liveBox);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 3);

    auto *centralLayout = new QVBoxLayout();
    centralLayout->setContentsMargins(6, 6, 6, 6);
    centralLayout->addWidget(splitter, /*stretch=*/1);
    centralLayout->addWidget(transportBox);

    auto *central = new QWidget(this);
    central->setLayout(centralLayout);
    setCentralWidget(central);

    connect(m_model, &ScheduleModel::scheduleChanged, this, &OperatorWindow::onScheduleChanged);
    connect(m_model, &ScheduleModel::liveContentChanged, this, &OperatorWindow::onLiveContentChanged);
    connect(m_model, &ScheduleModel::previewChanged, this, &OperatorWindow::onPreviewChanged);

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
    // Selecting a row only stages it for preview -- it must never touch
    // what's live. This is the core HCI guarantee of the preview/live split.
    if (row >= 0)
        m_model->setPreviewIndex(row);
}

void OperatorWindow::onScheduleItemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    m_model->goLiveWithPreview();
}

void OperatorWindow::onGoLiveClicked()
{
    m_model->goLiveWithPreview();
}

void OperatorWindow::onScheduleChanged()
{
    rebuildScheduleList();
}

void OperatorWindow::onLiveContentChanged()
{
    refreshScheduleHighlighting();
}

void OperatorWindow::onPreviewChanged()
{
    QSignalBlocker blocker(m_scheduleList);
    m_scheduleList->setCurrentRow(m_model->previewIndex());
    refreshScheduleHighlighting();
}

void OperatorWindow::rebuildScheduleList()
{
    QSignalBlocker blocker(m_scheduleList);
    m_scheduleList->clear();
    for (int i = 0; i < m_model->count(); ++i) {
        m_scheduleList->addItem(m_model->slideAt(i).label);
    }
    m_scheduleList->setCurrentRow(m_model->previewIndex());
    refreshScheduleHighlighting();
}

void OperatorWindow::refreshScheduleHighlighting()
{
    // Mark the live row distinctly from the preview selection: preview is
    // shown via the normal selection highlight, live gets an explicit
    // "ON AIR" marker so it's unambiguous even when preview and live
    // happen to be the same row.
    for (int i = 0; i < m_scheduleList->count(); ++i) {
        QListWidgetItem *item = m_scheduleList->item(i);
        const QString baseLabel = m_model->slideAt(i).label;
        if (i == m_model->liveIndex() && !m_model->isBlackout()) {
            item->setText(tr("\u25CF ON AIR  %1").arg(baseLabel));
            item->setForeground(QColor("#e74c3c"));
        } else {
            item->setText(baseLabel);
            item->setForeground(QApplication::palette().text().color());
        }
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
    // fullscreen. Otherwise fall back to a normal window so this is still
    // usable on a single-monitor dev machine.
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

    // Whenever the operator window becomes active again, bring the output
    // window back above whatever was covering it. raise() only reorders
    // stacking -- it does not steal keyboard focus, so the operator
    // window keeps receiving keypresses uninterrupted.
    if (event->type() == QEvent::ActivationChange && isActiveWindow()) {
        if (m_outputWindow->isVisible())
            m_outputWindow->raise();
    }
}

void OperatorWindow::closeEvent(QCloseEvent *event)
{
    // Treat this as one application: closing the control window must
    // never leave a bare output window orphaned on screen.
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
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_model->goLiveWithPreview();
        break;
    case Qt::Key_B:
        m_blackButton->toggle();
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }
    event->accept();
}