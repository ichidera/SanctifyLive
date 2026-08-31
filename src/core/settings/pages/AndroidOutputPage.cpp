#include "AndroidOutputPage.h"

#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "OutputSettingsPage.h"

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

AndroidOutputPage::AndroidOutputPage(OutputProfile *profile, SlideServer *slideServer, QWidget *parent)
    : QWidget(parent), m_slideServer(slideServer)
{
    m_devicesSummaryLabel = new QLabel(this);
    m_devicesSummaryLabel->setStyleSheet("color: #888;");

    m_devicesTable = new QTableWidget(0, 3, this);
    m_devicesTable->setHorizontalHeaderLabels({tr("Device"), tr("Role"), tr("Resolution")});
    m_devicesTable->horizontalHeader()->setStretchLastSection(true);
    m_devicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_devicesTable->verticalHeader()->setVisible(false);
    m_devicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_devicesTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_devicesTable->setMaximumHeight(140);

    m_wakeAllButton = new QPushButton(tr("Wake All Displays"), this);
    m_wakeAllButton->setToolTip(
        tr("Force every connected device's screen on, even if it's locked or asleep"));
    connect(m_wakeAllButton, &QPushButton::clicked, this, &AndroidOutputPage::onWakeAllClicked);

    auto *devicesGroupLayout = new QVBoxLayout;
    devicesGroupLayout->addWidget(m_devicesSummaryLabel);
    devicesGroupLayout->addWidget(m_devicesTable);
    devicesGroupLayout->addWidget(m_wakeAllButton, 0, Qt::AlignLeft);

    m_outputSettings = new OutputSettingsPage(profile, this);
    connect(m_outputSettings, &OutputSettingsPage::previewRelevantChanged,
            this, &AndroidOutputPage::previewRelevantChanged);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(devicesGroupLayout);
    layout->addWidget(m_outputSettings, 1);

    connect(m_slideServer, &SlideServer::clientsChanged, this, &AndroidOutputPage::onClientsChanged);
    refreshDevicesTable(m_slideServer->clients());
}

void AndroidOutputPage::onClientsChanged(const QVector<SlideServer::ClientInfo> &clients)
{
    refreshDevicesTable(clients);
}

void AndroidOutputPage::onWakeAllClicked()
{
    m_slideServer->wakeAll();
}

void AndroidOutputPage::refreshDevicesTable(const QVector<SlideServer::ClientInfo> &clients)
{
    m_devicesTable->setRowCount(clients.size());
    for (int row = 0; row < clients.size(); ++row) {
        const SlideServer::ClientInfo &info = clients.at(row);
        m_devicesTable->setItem(row, 0, new QTableWidgetItem(info.address));
        m_devicesTable->setItem(row, 1, new QTableWidgetItem(roleLabel(info.role)));
        m_devicesTable->setItem(row, 2, new QTableWidgetItem(resolutionLabel(info.resolution)));
    }

    m_devicesSummaryLabel->setText(clients.isEmpty()
        ? tr("No devices connected.")
        : (clients.size() == 1 ? tr("1 device connected.") : tr("%1 devices connected.").arg(clients.size())));
    m_wakeAllButton->setEnabled(!clients.isEmpty());
}
