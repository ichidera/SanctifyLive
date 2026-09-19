#pragma once

#include <QPixmap>
#include <QWidget>

class QVariantAnimation;
class ScheduleModel;

// OutputWindow is what actually gets projected/displayed to the
// congregation -- the OUTPUT WINDOW in OperatorWindow's panel glossary
// (see OperatorWindow.h). It observes a ScheduleModel and repaints
// whenever the live content changes -- it never receives direct
// commands from the operator UI. Keeping this one-directional (model ->
// output) is what lets future control surfaces (web remote, stage view)
// drive the same output without this class needing to know they exist.
//
// Two instances of this class exist at runtime, both watching the same
// ScheduleModel so they can never disagree: OperatorWindow's
// m_outputWindow (the real, possibly-fullscreen congregation-facing
// window) and m_livePreview (an embedded instance that IS the LIVE
// panel in the Main Row). This class has no idea which role it's
// playing in a given instance -- that distinction lives entirely in how
// OperatorWindow shows/positions each one.
//
// RESOLUTION INDEPENDENCE: this class never renders content itself. It
// asks SlideRenderer for a QPixmap at kDesignResolution (see
// RenderResolution.h) and caches it in m_canvas -- that render only
// happens when the live content actually changes. paintEvent's only job
// is to scale that one cached pixmap into whatever physical rect() this
// widget currently has, letterboxing if the aspect ratio doesn't match.
// This is also why the SAME class works both as the real projector
// output and as OperatorWindow's small embedded live preview: both are
// just different physical rect()s scaling the same bytes. See
// SlideRenderer.h and the module-level design note for the full
// rationale.
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
//
// TRANSITIONS: when HardwareSettings::currentPreset().fadeTransitions is
// on (see src/hardware/ and ui/HardwareSetupDialog), changing from one
// real slide to another crossfades over fadeDurationMs instead of
// cutting instantly -- the one feature this hardware-tier system
// actually gates today, chosen because it's the most visible, lowest-
// risk way to prove the tier recommendation means something rather than
// being a settings screen that edits values nothing reads. Deliberately
// NOT applied when the new or previous frame is a blackout (a null
// Slide): a "cut to black" action should stay an immediate, unambiguous
// cut regardless of the fade setting, the same reasoning ProPresenter/
// EasyWorship's own Black button follows -- an operator hitting that
// button in an emergency shouldn't watch it ease out over half a
// second.
class OutputWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OutputWindow(ScheduleModel *model, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onLiveContentChanged();

private:
    ScheduleModel *m_model;

    // The current frame, pre-rendered at kDesignResolution. paintEvent
    // only ever scales this -- it never draws a Slide's content
    // directly. Re-rendered exactly once per liveContentChanged signal,
    // never on resize.
    QPixmap m_canvas;

    // The frame being faded FROM; only meaningful while m_fadeProgress <
    // 1.0. Left null when no crossfade is in progress, so paintEvent's
    // "am I mid-fade" check is just "is this non-null" rather than a
    // separate bool to keep in sync.
    QPixmap m_previousCanvas;

    // 0.0 = paintEvent shows only m_previousCanvas; 1.0 = shows only
    // m_canvas; in between, both are drawn with m_canvas at this opacity
    // over m_previousCanvas underneath. Driven by m_fadeAnimation.
    qreal m_fadeProgress = 1.0;

    QVariantAnimation *m_fadeAnimation = nullptr; // owned; parented to this
};
