#include "OutputSettingsPage.h"

#include <QTabWidget>
#include <QVBoxLayout>

#include "OutputContentTabs.h"

OutputSettingsPage::OutputSettingsPage(OutputProfile *profile, QWidget *parent)
    : QWidget(parent), m_profile(profile)
{
    m_tabs = new QTabWidget(this);

    auto *generalTab = new OutputGeneralTab(m_profile, m_tabs);
    connect(generalTab, &OutputGeneralTab::previewRelevantChanged, this, &OutputSettingsPage::previewRelevantChanged);

    m_tabs->addTab(generalTab, tr("General"));
    m_tabs->addTab(new OutputSongTab(m_profile, m_tabs), tr("Song"));
    m_tabs->addTab(new OutputScriptureTab(m_profile, m_tabs), tr("Scripture"));
    m_tabs->addTab(new OutputPresentationTab(m_profile, m_tabs), tr("Presentation"));
    m_tabs->addTab(new OutputTransitionsTab(m_profile, m_tabs), tr("Transitions"));
    m_tabs->addTab(new OutputAlertsTab(m_profile, m_tabs), tr("Alerts"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tabs);
}
