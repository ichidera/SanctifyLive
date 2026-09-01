#ifndef SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTPREVIEWTHUMBNAIL_H_
#define SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTPREVIEWTHUMBNAIL_H_

#include <QWidget>

#include "../OutputProfile.h"

// The small "what will actually go out" thumbnail that sits under the
// category list in SettingsWindow's left column, regardless of which
// category is selected.
//
// This used to just draw a handful of fixed sample lines over a dark
// mockup box plus a resolution badge -- a good-enough placeholder before
// there was a shared way to actually render a Slide. Now it paints a
// representative sample slide through the exact same SlideRenderer used
// by the real congregation-facing OutputWindow and every content-tab
// preview (Media/Scriptures/Songs/Themes), fed the OutputProfile of
// whichever category the operator currently has open -- so General's
// margins, the chosen font, and the destination's real aspect ratio all
// show up here immediately, not just its resolution number.
class OutputPreviewThumbnail : public QWidget
{
    Q_OBJECT

public:
    explicit OutputPreviewThumbnail(QWidget *parent = nullptr);

    QSize sizeHint() const override;

public slots:
    // `profile` is the OutputProfile belonging to whichever sidebar
    // category currently has focus (nullptr for categories that aren't
    // profile-backed at all, e.g. Service Intervals/Advanced -- shown
    // dimmed with no badge). See SettingsWindow::refreshPreviewThumbnail.
    void setProfile(const OutputProfile *profile);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRect frameRect() const;

    QStringList m_sampleLines{
        tr("song text line one"),
        tr("song text line two"),
        tr("song text line three"),
        tr("song text line four"),
    };
    OutputProfile m_profile;
    bool m_hasProfile = false;
    bool m_enabledLook = true;
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTPREVIEWTHUMBNAIL_H_
