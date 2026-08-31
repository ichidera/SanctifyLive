#include "ServiceIntervalsPage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QGridLayout>

ServiceIntervalsPage::ServiceIntervalsPage(QWidget *parent) : QWidget(parent)
{
    auto *group = new QGroupBox(tr("Between Services"), this);

    m_clearScheduleBetweenServices = new QCheckBox(
        tr("Automatically clear the schedule after a gap in activity"), group);
    m_warnBeforeClearing = new QCheckBox(tr("Ask for confirmation before clearing"), group);
    m_warnBeforeClearing->setChecked(true);

    m_serviceGapHours = new QSpinBox(group);
    m_serviceGapHours->setRange(1, 72);
    m_serviceGapHours->setSuffix(tr(" hours"));
    m_serviceGapHours->setValue(6);

    auto *layout = new QGridLayout(group);
    layout->addWidget(m_clearScheduleBetweenServices, 0, 0, 1, 2);
    layout->addWidget(m_warnBeforeClearing, 1, 0, 1, 2);
    layout->addWidget(new QLabel(tr("Consider a gap of at least")), 2, 0);
    layout->addWidget(m_serviceGapHours, 2, 1);
    layout->setColumnStretch(1, 1);

    connect(m_clearScheduleBetweenServices, &QCheckBox::toggled, this, [this](bool on) {
        m_warnBeforeClearing->setEnabled(on);
        m_serviceGapHours->setEnabled(on);
    });
    m_warnBeforeClearing->setEnabled(m_clearScheduleBetweenServices->isChecked());
    m_serviceGapHours->setEnabled(m_clearScheduleBetweenServices->isChecked());

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(group);
    outer->addStretch(1);
}
