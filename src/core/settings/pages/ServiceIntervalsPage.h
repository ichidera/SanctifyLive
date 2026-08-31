#ifndef SANCTIFYLIVE_CORE_SETTINGS_PAGES_SERVICEINTERVALSPAGE_H_
#define SANCTIFYLIVE_CORE_SETTINGS_PAGES_SERVICEINTERVALSPAGE_H_

#include <QWidget>

class QCheckBox;
class QSpinBox;

// The "Service Intervals" entry in the Options sidebar: controls around
// what happens to the schedule/output between services (e.g. Sunday
// morning vs. Wednesday night), as opposed to per-output rendering
// settings, which live under Main Output/Alternate Output/Foldback/
// Android instead. Kept as its own page/class since it's a natural,
// separately-testable unit -- not because it needs ScheduleModel today.
//
// NOTE: these fields are UI-only for now, same as the rest of this
// milestone's Options rework -- ScheduleModel doesn't yet have a concept
// of "service gap" to wire into. When that lands, this page is the only
// file that needs to change.
class ServiceIntervalsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceIntervalsPage(QWidget *parent = nullptr);

private:
    QCheckBox *m_clearScheduleBetweenServices;
    QCheckBox *m_warnBeforeClearing;
    QSpinBox *m_serviceGapHours;
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_PAGES_SERVICEINTERVALSPAGE_H_
