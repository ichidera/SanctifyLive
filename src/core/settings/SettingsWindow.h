#ifndef SANCTIFYLIVE_CORE_SETTINGSWINDOW_H_
#define SANCTIFYLIVE_CORE_SETTINGSWINDOW_H_

#include <QVector>
#include <QWidget>

#include "../remote/SlideServer.h"

class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;
class ScheduleModel;

// The expanded "Edit > Options" window. What used to be a single modal
// checkbox dialog is now a real, persistent (non-modal) top-level
// window with tabs -- General for app-wide preferences, and Displays
// for a live view of every connected Android device (role, address,
// reported resolution) plus the ability to wake one on demand.
//
// OperatorWindow owns exactly one instance, created lazily on first use
// and just re-shown/raised after that -- so settings persist across
// opens/closes within a session and the window doesn't lose its place.
class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(ScheduleModel *model, SlideServer *slideServer, QWidget *parent = nullptr);

private slots:
    void onClientsChanged(const QVector<SlideServer::ClientInfo> &clients);
    void onWakeAllClicked();

private:
    QWidget *buildGeneralTab();
    QWidget *buildDisplaysTab();
    void refreshDisplaysTable(const QVector<SlideServer::ClientInfo> &clients);

    ScheduleModel *m_model;
    SlideServer *m_slideServer;

    QCheckBox *m_autoAddCheck;
    QTableWidget *m_displaysTable;
    QLabel *m_displaysSummaryLabel;
    QPushButton *m_wakeAllButton;
};

#endif // SANCTIFYLIVE_CORE_SETTINGSWINDOW_H_
