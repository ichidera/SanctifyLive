#include "SettingsWindow.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include "ScheduleModel.h"

namespace {
QString roleLabel(SlideServer::Role role)
{
    return (role == SlideServer::Role::Phone) ? QObject::tr("Phone") : QObject::tr("Display");
}

QString resolutionLabel(const QSize &resolution)
{
    if (!resolution.isValid() || resolution.isEmpty())
        return QObject::tr("(unknown)");
    return QStringLiteral("%1 x %2").arg(resolution.width()).arg(resolution.height());
}
}

SettingsWindow::SettingsWindow(ScheduleModel *model, SlideServer *slideServer, QWidget *parent)
    : QWidget(parent), m_model(model), m_slideServer(slideServer)
{
    setWindowTitle(tr("Options"));
    setWindowFlag(Qt::Window); // a real top-level window, not embedded in OperatorWindow
    resize(560, 420);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildGeneralTab(), tr("General"));
    tabs->addTab(buildDisplaysTab(), tr("Displays"));

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);

    connect(m_slideServer, &SlideServer::clientsChanged, this, &SettingsWindow::onClientsChanged);
    refreshDisplaysTable(m_slideServer->clients());
}

QWidget *SettingsWindow::buildGeneralTab()
{
    auto *tab = new QWidget(this);

    m_autoAddCheck = new QCheckBox(
        tr("Automatically add media items to the schedule when sent live"), tab);
    m_autoAddCheck->setChecked(m_model->autoAddMediaToSchedule());
    // Applies immediately -- this is a live, persistent window now
    // rather than a modal OK/Cancel transaction, so there's no reason to
    // make the operator commit a second time.
    connect(m_autoAddCheck, &QCheckBox::toggled, m_model, &ScheduleModel::setAutoAddMediaToSchedule);

    auto *layout = new QVBoxLayout(tab);
    layout->addWidget(m_autoAddCheck);
    layout->addStretch(1);
    return tab;
}

QWidget *SettingsWindow::buildDisplaysTab()
{
    auto *tab = new QWidget(this);

    m_displaysSummaryLabel = new QLabel(tab);
    m_displaysSummaryLabel->setStyleSheet("color: #888;");

    m_displaysTable = new QTableWidget(0, 3, tab);
    m_displaysTable->setHorizontalHeaderLabels({tr("Device"), tr("Role"), tr("Resolution")});
    m_displaysTable->horizontalHeader()->setStretchLastSection(true);
    m_displaysTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_displaysTable->verticalHeader()->setVisible(false);
    m_displaysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_displaysTable->setSelectionMode(QAbstractItemView::NoSelection);

    m_wakeAllButton = new QPushButton(tr("Wake All Displays"), tab);
    m_wakeAllButton->setToolTip(
        tr("Force every connected device's screen on, even if it's locked or asleep"));
    connect(m_wakeAllButton, &QPushButton::clicked, this, &SettingsWindow::onWakeAllClicked);

    auto *layout = new QVBoxLayout(tab);
    layout->addWidget(m_displaysSummaryLabel);
    layout->addWidget(m_displaysTable, 1);
    layout->addWidget(m_wakeAllButton, 0, Qt::AlignLeft);
    return tab;
}

void SettingsWindow::onClientsChanged(const QVector<SlideServer::ClientInfo> &clients)
{
    refreshDisplaysTable(clients);
}

void SettingsWindow::onWakeAllClicked()
{
    m_slideServer->wakeAll();
}

void SettingsWindow::refreshDisplaysTable(const QVector<SlideServer::ClientInfo> &clients)
{
    m_displaysTable->setRowCount(clients.size());
    for (int row = 0; row < clients.size(); ++row) {
        const SlideServer::ClientInfo &info = clients.at(row);
        m_displaysTable->setItem(row, 0, new QTableWidgetItem(info.address));
        m_displaysTable->setItem(row, 1, new QTableWidgetItem(roleLabel(info.role)));
        m_displaysTable->setItem(row, 2, new QTableWidgetItem(resolutionLabel(info.resolution)));
    }

    m_displaysSummaryLabel->setText(clients.isEmpty()
        ? tr("No devices connected.")
        : (clients.size() == 1 ? tr("1 device connected.") : tr("%1 devices connected.").arg(clients.size())));
    m_wakeAllButton->setEnabled(!clients.isEmpty());
}
