#ifndef SANCTIFYLIVE_CORE_SETTINGSWINDOW_H_
#define SANCTIFYLIVE_CORE_SETTINGSWINDOW_H_

#include <QWidget>

#include "../remote/SlideServer.h"
#include "OutputProfile.h"

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class ScheduleModel;
class OutputSettingsPage;
class AndroidOutputPage;
class OutputPreviewThumbnail;

// The expanded "Edit > Options" window. What used to be a single modal
// checkbox dialog, then a two-tab (General/Displays) window, is now a
// full options window in the shape most operators will recognize from
// other live-production tools: a category sidebar on the left (Main
// Output, Alternate Output, Foldback, Android, Service Intervals,
// Advanced) with a small "what will actually go out" preview underneath
// it, and the selected category's settings on the right, committed with
// OK or thrown away with Cancel.
//
// Folder layout (src/core/settings/):
//   SettingsWindow.{h,cpp}        - this file: sidebar + stacked pages + OK/Cancel
//   OutputProfile.h               - plain-data settings shared by every output
//   pages/OutputSettingsPage      - the General/Song/Scripture/Presentation/
//                                   Transitions/Alerts tab strip, reused for
//                                   Main Output, Alternate Output, and Foldback
//   pages/OutputContentTabs       - the six small tab widgets that page is built from
//   pages/AndroidOutputPage       - device list + the same tab strip, for Android
//   pages/ServiceIntervalsPage    - the Service Intervals sidebar page
//   pages/AdvancedPage            - the Advanced sidebar page (old General tab)
//   pages/OutputPreviewThumbnail  - the small preview box under the sidebar
//
// Four destinations (Main Output/Alternate Output/Foldback/Android) share
// one OutputProfile struct and one OutputSettingsPage class rather than
// four bespoke pages -- adding a fifth output destination later means
// constructing another profile and reusing that page class, not writing
// new UI.
//
// OperatorWindow owns exactly one instance, created lazily on first use
// and just re-shown/raised after that -- so the window doesn't lose its
// place across opens/closes within a session. Because of that reuse,
// OK/Cancel here don't map onto construction/destruction: every
// OutputProfile is snapshotted on open, so Cancel has something to
// restore to, and OK re-takes that snapshot so the *next* open starts
// from what was just committed. Restoring rebuilds the output pages from
// the restored profiles rather than trying to reset every widget in
// every tab by hand -- see rebuildOutputPages().
class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(ScheduleModel *model, SlideServer *slideServer, QWidget *parent = nullptr);

signals:
    // Fired only on OK (see the class comment above for why) so
    // OperatorWindow can point the real congregation-facing OutputWindow
    // at whatever monitor was just chosen for Main Output.
    void mainOutputChanged(int monitorIndex, const QRect &position);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onCategoryChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void onOkClicked();
    void onCancelClicked();
    void refreshPreviewThumbnail();

private:
    enum Category { MainOutput, AlternateOutput, Foldback, Android, ServiceIntervals, Advanced };

    QWidget *buildSidebar();
    void rebuildOutputPages(); // (re)constructs the four OutputProfile-backed pages from current working profiles

    ScheduleModel *m_model;
    SlideServer *m_slideServer;

    QListWidget *m_categoryList;
    QStackedWidget *m_pages;
    OutputPreviewThumbnail *m_previewThumbnail;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    OutputSettingsPage *m_mainOutputPage = nullptr;
    OutputSettingsPage *m_alternateOutputPage = nullptr;
    OutputSettingsPage *m_foldbackPage = nullptr;
    AndroidOutputPage *m_androidPage = nullptr;
    QWidget *m_serviceIntervalsPage = nullptr;
    QWidget *m_advancedPage = nullptr;

    OutputProfile m_mainOutputProfile;
    OutputProfile m_alternateOutputProfile;
    OutputProfile m_foldbackProfile;
    OutputProfile m_androidProfile;

    OutputProfile m_committedMainOutputProfile;
    OutputProfile m_committedAlternateOutputProfile;
    OutputProfile m_committedFoldbackProfile;
    OutputProfile m_committedAndroidProfile;
};

#endif // SANCTIFYLIVE_CORE_SETTINGSWINDOW_H_
