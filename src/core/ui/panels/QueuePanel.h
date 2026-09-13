#pragma once

#include <QWidget>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class ScheduleModel;

// QueuePanel is the operator-facing view of the full schedule, styled to
// match the "Queue" card list from the design: each row shows a slide's
// label and a preview of its text, with distinct highlighting for
// whichever row is staged (preview) vs currently on air (live).
//
// Deliberately owns no state of its own -- it's a thin view over
// ScheduleModel. Clicking a row stages it (setPreviewIndex), it never
// jumps live output directly; double-clicking (or pressing Enter) stages
// AND cuts live (goLive), matching the "single click previews, you
// explicitly commit it" behavior the Preview/Live split is for.
class QueuePanel : public QWidget
{
    Q_OBJECT

public:
    explicit QueuePanel(ScheduleModel *model, QWidget *parent = nullptr);

signals:
    // Bubbled up so OperatorWindow can route "Add Slide" through whatever
    // input flow it wants (a dialog today, a song/scripture picker later)
    // without QueuePanel needing to know about that flow.
    void addSlideRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onScheduleChanged();
    void onCursorsChanged();
    void onItemClicked(QListWidgetItem *item);
    void onItemDoubleClicked(QListWidgetItem *item);
    void onRemoveClicked();

private:
    void rebuild();
    void refreshHighlights();

    ScheduleModel *m_model;
    QListWidget *m_list;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
};
