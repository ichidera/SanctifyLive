#ifndef SANCTIFYLIVE_CORE_OUTPUT_LIVEAPPEARANCEPREVIEW_H_
#define SANCTIFYLIVE_CORE_OUTPUT_LIVEAPPEARANCEPREVIEW_H_

#include <QWidget>

#include "../schedule/Slide.h"
#include "../settings/OutputProfile.h"

// The "what will this actually look like on screen" preview used by
// every content-type tab of the bottom resource library (Media,
// Scriptures, Songs, Themes) that lets the operator pick something
// before sending it live.
//
// This is deliberately NOT a scaled-up thumbnail of whatever image or
// text was selected -- it renders through the exact same SlideRenderer
// the real congregation-facing OutputWindow uses, into a box that always
// has the *destination's actual configured aspect ratio*
// (OutputProfile::outputPosition), letterboxed/pillarboxed to fit
// whatever space this widget happens to have. That combination is what
// makes the "if something is cut off here, it's cut off on screen, not
// just off the edge of this panel" guarantee true: the frame this widget
// hands to SlideRenderer has the real screen's shape, so a cover-fit
// image crop or a too-long line of text is cropped/laid out exactly the
// way it would be on the real output, just scaled down uniformly to fit
// here.
//
// Every panel that owns one of these should also forward
// setOutputProfile() from whatever tells it the current Main Output
// profile (see MediaLibraryPanel::setOutputProfile / OperatorWindow),
// so the preview stays in sync as the operator changes resolution,
// margins, or the output font in Options > Main Output.
class LiveAppearancePreview : public QWidget
{
    Q_OBJECT

public:
    explicit LiveAppearancePreview(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    // Shows `slide` as it would actually be projected.
    void setSlide(const Slide &slide);
    // Nothing selected yet -- shows an empty frame with a placeholder
    // message instead of stale content from a previous selection.
    void clearSlide();
    // Which destination's resolution/margins/font this preview should
    // represent. Defaults to a plain OutputProfile() (1920x1080, no
    // margins) until told otherwise, so panels look sensible even before
    // Options > Main Output has ever been opened this session.
    void setProfile(const OutputProfile &profile);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRect frameRect() const;

    Slide m_slide;
    bool m_hasSlide = false;
    OutputProfile m_profile;
};

#endif // SANCTIFYLIVE_CORE_OUTPUT_LIVEAPPEARANCEPREVIEW_H_
