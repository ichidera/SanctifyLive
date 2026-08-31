#ifndef SANCTIFYLIVE_CORE_SETTINGS_PAGES_ADVANCEDPAGE_H_
#define SANCTIFYLIVE_CORE_SETTINGS_PAGES_ADVANCEDPAGE_H_

#include <QWidget>

class ScheduleModel;
class QCheckBox;

// The "Advanced" entry in the Options sidebar: app-wide preferences that
// don't belong to any single output and aren't frequent enough to need
// their own top-level sidebar category. This is what used to be the
// entire contents of the old single-page SettingsWindow's "General" tab
// (see git history) -- it moves here unchanged as the rest of the window
// grows around it.
//
// Unlike the OutputProfile-backed pages, this one still applies its
// checkbox to the model immediately rather than waiting for OK. That's
// deliberate: ScheduleModel::autoAddMediaToSchedule has no "pending"
// concept of its own, and toggling it has no visible side effect the
// operator could want to preview-then-discard the way output framing
// does, so there's nothing Cancel would need to undo.
class AdvancedPage : public QWidget
{
    Q_OBJECT

public:
    explicit AdvancedPage(ScheduleModel *model, QWidget *parent = nullptr);

private:
    ScheduleModel *m_model;
    QCheckBox *m_autoAddCheck;
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_PAGES_ADVANCEDPAGE_H_
