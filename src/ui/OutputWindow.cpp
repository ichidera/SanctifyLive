#include "ui/OutputWindow.h"

#include <QPainter>
#include <QPaintEvent>
#include <QVariantAnimation>

#include "core/model/ScheduleModel.h"
#include "core/render/RenderResolution.h"
#include "core/render/SlideRenderer.h"
#include "hardware/HardwareSettings.h"

OutputWindow::OutputWindow(ScheduleModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setWindowTitle(tr("SanctifyLive - Output"));

    // This floor is just "don't let the window collapse to nothing" --
    // it has no bearing on layout quality any more. Content is always
    // laid out at kDesignResolution and scaled down to whatever size
    // this widget ends up being, so there's no size below which text
    // wrapping degrades.
    setMinimumSize(160, 90);

    // Plain black until told otherwise -- an output window should never
    // show stale or garbage content before the model has spoken.
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    m_fadeAnimation = new QVariantAnimation(this);
    connect(m_fadeAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_fadeProgress = value.toReal();
        update();
    });
    // Once the target frame is fully shown, the previous one is no
    // longer needed for painting -- dropping it frees the memory rather
    // than holding an extra full-resolution pixmap alive indefinitely
    // between transitions.
    connect(m_fadeAnimation, &QVariantAnimation::finished, this, [this]() { m_previousCanvas = QPixmap(); });

    connect(m_model, &ScheduleModel::liveContentChanged,
            this, &OutputWindow::onLiveContentChanged);

    // Render the current state immediately so the first paintEvent
    // (which may fire before any liveContentChanged signal, e.g. if the
    // model already had slides when this window was constructed) has a
    // real frame to scale instead of a null pixmap.
    onLiveContentChanged();
}

void OutputWindow::onLiveContentChanged()
{
    // This is the ONLY place a new frame gets rendered. It happens once
    // per actual content change, at a fixed kDesignResolution -- never
    // in paintEvent, and never keyed off this widget's own size. Resizing
    // the window (dragging it, moving it to a different projector,
    // shrinking it to a preview thumbnail) triggers Qt resize/paint
    // events, NOT this slot, so it can never cause a re-render.
    const Slide *newSlide = m_model->currentSlide();
    const QPixmap newCanvas = SlideRenderer::render(newSlide, kDesignResolution);
    const FeaturePreset preset = HardwareSettings::currentPreset();

    // A crossfade needs something real on both ends: a previous frame to
    // fade FROM (m_canvas starts null, so the very first frame never
    // fades in from nothing) and a real slide to fade TO (a blackout
    // always cuts instantly -- see this class's doc comment for why).
    const bool canFade =
        preset.fadeTransitions && preset.fadeDurationMs > 0 && !m_canvas.isNull() && newSlide != nullptr;

    m_fadeAnimation->stop();
    if (canFade) {
        m_previousCanvas = m_canvas;
        m_canvas = newCanvas;
        m_fadeProgress = 0.0;
        m_fadeAnimation->setDuration(preset.fadeDurationMs);
        m_fadeAnimation->setStartValue(0.0);
        m_fadeAnimation->setEndValue(1.0);
        m_fadeAnimation->start();
    } else {
        m_canvas = newCanvas;
        m_fadeProgress = 1.0;
        m_previousCanvas = QPixmap();
    }
    update(); // schedule a repaint; Qt coalesces repeated calls automatically
}

void OutputWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Fill with black first so any letterboxing (when this widget's
    // aspect ratio doesn't match kDesignResolution's) shows clean black
    // bars rather than stale content or undefined background.
    painter.fillRect(rect(), Qt::black);

    if (m_canvas.isNull())
        return;

    // Scale the pre-rendered canvas to fit this widget's PHYSICAL rect,
    // preserving aspect ratio. This is the entire "adapt to whatever
    // screen we're actually on" logic -- no text layout, no font-size
    // math, no knowledge of Slide happens here. The exact same m_canvas
    // bytes get this treatment whether this OutputWindow is fullscreen
    // on a 1024x768 projector or a 200px-tall embedded preview in
    // OperatorWindow.
    const QSize scaledSize = m_canvas.size().scaled(size(), Qt::KeepAspectRatio);
    QRect targetRect(QPoint(0, 0), scaledSize);
    targetRect.moveCenter(rect().center());

    if (m_fadeProgress < 1.0 && !m_previousCanvas.isNull()) {
        // Mid-crossfade: the outgoing frame underneath, the incoming
        // frame on top at increasing opacity. Both pixmaps share the
        // same design resolution (kDesignResolution never changes mid-
        // session), so the same targetRect is correct for both.
        painter.drawPixmap(targetRect, m_previousCanvas);
        painter.setOpacity(m_fadeProgress);
        painter.drawPixmap(targetRect, m_canvas);
        painter.setOpacity(1.0);
    } else {
        painter.drawPixmap(targetRect, m_canvas);
    }
}
