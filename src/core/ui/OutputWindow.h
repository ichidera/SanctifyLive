#pragma once

#include <QPixmap>
#include <QWidget>

class ScheduleModel;

// OutputWindow displays one of ScheduleModel's two cursors -- Live or
// Preview (see ScheduleModel's class comment) -- and re-renders only
// when THAT cursor's corresponding signal fires. It never receives
// direct commands from the operator UI; keeping this one-directional
// (model -> output) is what lets future control surfaces (web remote,
// stage view) drive the same output without this class needing to know
// they exist.
//
// This is also why the exact same class is used for three different
// on-screen roles: the real congregation-facing projector output, the
// small embedded "Live" pane, and the small embedded "Preview" pane --
// they differ only in which Source they were constructed with and what
// physical rect() they end up in.
//
// RESOLUTION INDEPENDENCE: this class never renders content itself. It
// asks SlideRenderer for a QPixmap at kDesignResolution (see
// RenderResolution.h) and caches it in m_canvas -- that render only
// happens when the relevant cursor actually changes. paintEvent's only
// job is to scale that one cached pixmap into whatever physical rect()
// this widget currently has, letterboxing if the aspect ratio doesn't
// match. See SlideRenderer.h and the module-level design note for the
// full rationale.
//
// NOTE on architecture: this is a plain QWidget with a paintEvent for
// milestone one, which is enough to prove the control loop end-to-end.
// Once media/video layers are introduced, this should become a
// QOpenGLWidget (or move to Qt Quick/QRhi) so slide composition is
// GPU-accelerated and can run its own render loop independent of the
// operator UI thread -- see the "Core render & output pipeline" step
// in the build plan. Swapping that in later shouldn't require changes
// outside this file, since everything else only talks to ScheduleModel,
// and the design-resolution/scale-to-physical contract is unaffected by
// what draws the canvas.
class OutputWindow : public QWidget
{
    Q_OBJECT

public:
    enum class Source {
        Live,   // ScheduleModel::currentSlide() / liveContentChanged
        Preview // ScheduleModel::previewSlide() / previewChanged
    };

    explicit OutputWindow(ScheduleModel *model, Source source = Source::Live,
                           QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onContentChanged();

private:
    ScheduleModel *m_model;
    Source m_source;

    // The current frame, pre-rendered at kDesignResolution. paintEvent
    // only ever scales this -- it never draws a Slide's content
    // directly. Re-rendered exactly once per relevant signal, never on
    // resize.
    QPixmap m_canvas;
};
