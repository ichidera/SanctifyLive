#ifndef SANCTIFYLIVE_CORE_HISTORYBAR_H_
#define SANCTIFYLIVE_CORE_HISTORYBAR_H_


#include <QWidget>

class ScheduleModel;
class QHBoxLayout;

// A thin, horizontally-scrollable strip of everything recently sent
// live -- independent of the Schedule. This exists because "what's been
// shown" and "what's planned" are different questions: the schedule is
// the pre-planned order, this bar is a quick way back to something that
// was already put on screen (planned or not), without it needing to
// live in the schedule at all.
class HistoryBar : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryBar(ScheduleModel *model, QWidget *parent = nullptr);

private slots:
    void rebuild();

private:
    ScheduleModel *m_model;
    QHBoxLayout *m_stripLayout;
};

#endif // SANCTIFYLIVE_CORE_HISTORYBAR_H_