#ifndef SANCTIFYLIVE_CORE_SETTINGS_PAGES_ANDROIDOUTPUTPAGE_H_
#define SANCTIFYLIVE_CORE_SETTINGS_PAGES_ANDROIDOUTPUTPAGE_H_

#include <QVector>
#include <QWidget>

#include "../../remote/SlideServer.h"
#include "../OutputProfile.h"

class QLabel;
class QPushButton;
class QTableWidget;
class OutputSettingsPage;

// The "Android" entry in the Options sidebar. Android is a real output
// destination -- it gets the same General/Song/Scripture/Presentation/
// Transitions/Alerts tab strip as Main Output, Alternate Output, and
// Foldback, via one shared OutputSettingsPage -- but it's also the one
// output where the operator needs to see *what's actually plugged in*
// (there's no OS-level "detect monitors" for phones on a network), so
// this page puts a live connected-devices table above that shared strip.
// This replaces the old standalone "Displays" tab that used to live
// directly on SettingsWindow.
class AndroidOutputPage : public QWidget
{
    Q_OBJECT

public:
    explicit AndroidOutputPage(OutputProfile *profile, SlideServer *slideServer, QWidget *parent = nullptr);

signals:
    void previewRelevantChanged();

private slots:
    void onClientsChanged(const QVector<SlideServer::ClientInfo> &clients);
    void onWakeAllClicked();

private:
    void refreshDevicesTable(const QVector<SlideServer::ClientInfo> &clients);

    SlideServer *m_slideServer;
    OutputSettingsPage *m_outputSettings;
    QTableWidget *m_devicesTable;
    QLabel *m_devicesSummaryLabel;
    QPushButton *m_wakeAllButton;
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_PAGES_ANDROIDOUTPUTPAGE_H_
