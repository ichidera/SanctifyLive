#ifndef SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTSETTINGSPAGE_H_
#define SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTSETTINGSPAGE_H_

#include <QWidget>

#include "../OutputProfile.h"

class QTabWidget;
class OutputGeneralTab;

// One page of the Options window: the General/Song/Scripture/Presentation/
// Transitions/Alerts tab strip, all editing a single OutputProfile. This is
// the page used for Main Output, Alternate Output, and Foldback directly,
// and embedded inside AndroidOutputPage (alongside the device list) for
// Android -- see pages/AndroidOutputPage.h. One class, four destinations.
class OutputSettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit OutputSettingsPage(OutputProfile *profile, QWidget *parent = nullptr);

    OutputProfile *profile() const { return m_profile; }

signals:
    // Bubbles up from OutputGeneralTab so SettingsWindow can refresh the
    // shared preview thumbnail whenever this page's monitor/size changes,
    // without SettingsWindow reaching into the tab strip directly.
    void previewRelevantChanged();

private:
    OutputProfile *m_profile;
    QTabWidget *m_tabs;
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTSETTINGSPAGE_H_
