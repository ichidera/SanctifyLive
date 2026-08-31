#include "SettingsWindow.h"

#include <QCheckBox>
#include <QColor>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShowEvent>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "../common/IconFactory.h"
#include <QGuiApplication>
#include <QScreen>
#include "pages/AdvancedPage.h"
#include "pages/AndroidOutputPage.h"
#include "pages/OutputPreviewThumbnail.h"
#include "pages/OutputSettingsPage.h"
#include "pages/ServiceIntervalsPage.h"

SettingsWindow::SettingsWindow(ScheduleModel *model, SlideServer *slideServer, QWidget *parent)
    : QWidget(parent), m_model(model), m_slideServer(slideServer)
{
    setWindowTitle(tr("Options"));
    setWindowFlag(Qt::Window); // a real top-level window, not embedded in OperatorWindow
    resize(820, 560);

    m_mainOutputProfile.displayName = tr("Main Output");
    m_alternateOutputProfile.displayName = tr("Alternate Output");
    m_foldbackProfile.displayName = tr("Foldback");
    m_androidProfile.displayName = tr("Android");

    // Main Output defaults to a second monitor when one exists (the
    // classic one-computer, two-screen setup), matching the guess
    // OperatorWindow used to hardcode before this window could configure
    // it; the other three start unassigned ("None") until the operator
    // picks a monitor for them.
    const QList<QScreen *> screens = QGuiApplication::screens();
    const int defaultMainOutputMonitor = (screens.size() > 1) ? 1 : 0;
    m_mainOutputProfile.monitorIndex = defaultMainOutputMonitor;
    if (defaultMainOutputMonitor < screens.size())
        m_mainOutputProfile.outputPosition = screens.at(defaultMainOutputMonitor)->geometry();
    m_alternateOutputProfile.monitorIndex = -1;
    m_foldbackProfile.monitorIndex = -1;
    m_androidProfile.monitorIndex = -1;

    // buildSidebar() creates m_categoryList, which rebuildOutputPages()
    // below reads (to preserve the current row across a Cancel-triggered
    // rebuild) -- so the sidebar has to exist first.
    QWidget *sidebar = buildSidebar();

    m_pages = new QStackedWidget(this);

    // Service Intervals and Advanced aren't OutputProfile-backed, so they
    // are built once here rather than in rebuildOutputPages().
    m_serviceIntervalsPage = new ServiceIntervalsPage(m_pages);
    m_advancedPage = new AdvancedPage(m_model, m_pages);

    rebuildOutputPages(); // fills stack indices MainOutput..Android
    m_pages->insertWidget(ServiceIntervals, m_serviceIntervalsPage);
    m_pages->insertWidget(Advanced, m_advancedPage);

    auto *mainArea = new QHBoxLayout;
    mainArea->addWidget(sidebar);
    mainArea->addWidget(m_pages, 1);

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    connect(m_okButton, &QPushButton::clicked, this, &SettingsWindow::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsWindow::onCancelClicked);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_okButton);
    buttonRow->addWidget(m_cancelButton);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(mainArea, 1);
    layout->addLayout(buttonRow);

    m_categoryList->setCurrentRow(MainOutput);
}

QWidget *SettingsWindow::buildSidebar()
{
    auto *sidebar = new QWidget(this);
    sidebar->setFixedWidth(210);

    m_categoryList = new QListWidget(sidebar);
    m_categoryList->setIconSize(QSize(20, 20));

    auto addCategory = [this](Category category, const QString &label, const QIcon &icon) {
        auto *item = new QListWidgetItem(icon, label);
        item->setData(Qt::UserRole, static_cast<int>(category));
        m_categoryList->addItem(item);
    };
    addCategory(MainOutput, tr("Main Output"), IconFactory::monitorOutput(QColor("#5dade2")));
    addCategory(AlternateOutput, tr("Alternate Output"), IconFactory::monitorOutput(QColor("#e67e22")));
    addCategory(Foldback, tr("Foldback"), IconFactory::monitorOutput(QColor("#2ecc71")));
    addCategory(Android, tr("Android"), IconFactory::androidRobot());
    addCategory(ServiceIntervals, tr("Service Intervals"), IconFactory::clock());
    addCategory(Advanced, tr("Advanced"), IconFactory::gear());
    connect(m_categoryList, &QListWidget::currentItemChanged, this, &SettingsWindow::onCategoryChanged);

    m_previewThumbnail = new OutputPreviewThumbnail(sidebar);

    // Whether a small always-on-top preview window mirrors Main Output
    // isn't implemented yet -- disabled rather than silently doing
    // nothing, matching the toolbar's "coming soon" actions elsewhere.
    auto *previewOutputCheck = new QCheckBox(tr("Preview Output"), sidebar);
    previewOutputCheck->setEnabled(false);
    previewOutputCheck->setToolTip(tr("Coming in a future update"));

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_categoryList, 1);
    layout->addWidget(m_previewThumbnail);
    layout->addWidget(previewOutputCheck);
    return sidebar;
}

void SettingsWindow::rebuildOutputPages()
{
    const int previousRow = m_categoryList ? m_categoryList->currentRow() : MainOutput;

    if (m_mainOutputPage) {
        m_pages->removeWidget(m_mainOutputPage);
        m_mainOutputPage->deleteLater();
    }
    m_mainOutputPage = new OutputSettingsPage(&m_mainOutputProfile, m_pages);
    connect(m_mainOutputPage, &OutputSettingsPage::previewRelevantChanged,
            this, &SettingsWindow::refreshPreviewThumbnail);
    m_pages->insertWidget(MainOutput, m_mainOutputPage);

    if (m_alternateOutputPage) {
        m_pages->removeWidget(m_alternateOutputPage);
        m_alternateOutputPage->deleteLater();
    }
    m_alternateOutputPage = new OutputSettingsPage(&m_alternateOutputProfile, m_pages);
    connect(m_alternateOutputPage, &OutputSettingsPage::previewRelevantChanged,
            this, &SettingsWindow::refreshPreviewThumbnail);
    m_pages->insertWidget(AlternateOutput, m_alternateOutputPage);

    if (m_foldbackPage) {
        m_pages->removeWidget(m_foldbackPage);
        m_foldbackPage->deleteLater();
    }
    m_foldbackPage = new OutputSettingsPage(&m_foldbackProfile, m_pages);
    connect(m_foldbackPage, &OutputSettingsPage::previewRelevantChanged,
            this, &SettingsWindow::refreshPreviewThumbnail);
    m_pages->insertWidget(Foldback, m_foldbackPage);

    if (m_androidPage) {
        m_pages->removeWidget(m_androidPage);
        m_androidPage->deleteLater();
    }
    m_androidPage = new AndroidOutputPage(&m_androidProfile, m_slideServer, m_pages);
    connect(m_androidPage, &AndroidOutputPage::previewRelevantChanged,
            this, &SettingsWindow::refreshPreviewThumbnail);
    m_pages->insertWidget(Android, m_androidPage);

    if (previousRow >= 0)
        m_pages->setCurrentIndex(previousRow);
    refreshPreviewThumbnail();
}

void SettingsWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Every time the window is (re-)shown, whatever is currently in the
    // working profiles becomes the new "committed" baseline -- so a
    // Cancel later in *this* session reverts to how things looked when
    // the operator opened Options just now, not to some stale state from
    // the app's very first launch.
    m_committedMainOutputProfile = m_mainOutputProfile;
    m_committedAlternateOutputProfile = m_alternateOutputProfile;
    m_committedFoldbackProfile = m_foldbackProfile;
    m_committedAndroidProfile = m_androidProfile;
}

void SettingsWindow::onCategoryChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    // m_categoryList can emit this the moment its first item is added
    // (that item becomes "current" automatically), which happens while
    // buildSidebar() is still running -- before m_pages exists yet.
    if (!current || !m_pages)
        return;
    m_pages->setCurrentIndex(current->data(Qt::UserRole).toInt());
    refreshPreviewThumbnail();
}

void SettingsWindow::onOkClicked()
{
    m_committedMainOutputProfile = m_mainOutputProfile;
    m_committedAlternateOutputProfile = m_alternateOutputProfile;
    m_committedFoldbackProfile = m_foldbackProfile;
    m_committedAndroidProfile = m_androidProfile;
    emit mainOutputChanged(m_committedMainOutputProfile.monitorIndex, m_committedMainOutputProfile.outputPosition);
    close();
}

void SettingsWindow::onCancelClicked()
{
    m_mainOutputProfile = m_committedMainOutputProfile;
    m_alternateOutputProfile = m_committedAlternateOutputProfile;
    m_foldbackProfile = m_committedFoldbackProfile;
    m_androidProfile = m_committedAndroidProfile;
    // The tab widgets only read their profile once, at construction, so
    // reverting the underlying data means rebuilding them rather than
    // hunting down every QSpinBox/QComboBox to reset by hand.
    rebuildOutputPages();
    close();
}

void SettingsWindow::refreshPreviewThumbnail()
{
    const OutputProfile *profile = nullptr;
    switch (m_pages->currentIndex()) {
    case MainOutput: profile = &m_mainOutputProfile; break;
    case AlternateOutput: profile = &m_alternateOutputProfile; break;
    case Foldback: profile = &m_foldbackProfile; break;
    case Android: profile = &m_androidProfile; break;
    default: break;
    }

    if (!profile) {
        m_previewThumbnail->setEnabledLook(false);
        m_previewThumbnail->setResolutionLabel(QString());
        return;
    }

    m_previewThumbnail->setEnabledLook(profile->monitorIndex >= 0);
    m_previewThumbnail->setResolutionLabel(profile->monitorIndex >= 0
        ? QStringLiteral("%1x%2").arg(profile->outputPosition.width()).arg(profile->outputPosition.height())
        : tr("None"));
}
