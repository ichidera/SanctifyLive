#include "AdvancedPage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

#include "../../schedule/ScheduleModel.h"

AdvancedPage::AdvancedPage(ScheduleModel *model, QWidget *parent) : QWidget(parent), m_model(model)
{
    auto *group = new QGroupBox(tr("Schedule"), this);

    m_autoAddCheck = new QCheckBox(
        tr("Automatically add media items to the schedule when sent live"), group);
    m_autoAddCheck->setChecked(m_model->autoAddMediaToSchedule());
    connect(m_autoAddCheck, &QCheckBox::toggled, m_model, &ScheduleModel::setAutoAddMediaToSchedule);

    auto *groupLayout = new QVBoxLayout(group);
    groupLayout->addWidget(m_autoAddCheck);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(group);
    outer->addStretch(1);
}
