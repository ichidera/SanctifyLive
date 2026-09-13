#include "core/ui/OutputWindow.h"

#include <QPainter>
#include <QPaintEvent>

#include "core/model/ScheduleModel.h"
#include "core/render/RenderResolution.h"
#include "core/render/SlideRenderer.h"

OutputWindow::OutputWindow(ScheduleModel *model, Source source, QWidget *parent)
    : QWidget(parent), m_model(model), m_source(source)
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

    // Only listen to the ONE signal that corresponds to this instance's
    // Source. A Preview pane must never repaint on liveContentChanged
    // (and vice versa) -- that's what keeps the two panes independent.
    if (m_source == Source::Live) {
        connect(m_model, &ScheduleModel::liveContentChanged,
                this, &OutputWindow::onContentChanged);
    } else {
        connect(m_model, &ScheduleModel::previewChanged,
                this, &OutputWindow::onContentChanged);
    }

    // Render the current state immediately so the first paintEvent
    // (which may fire before any signal, e.g. if the model already had
    // slides when this window was constructed) has a real frame to
    // scale instead of a null pixmap.
    onContentChanged();
}

void OutputWindow::onContentChanged()
{
    // This is the ONLY place a new frame gets rendered. It happens once
    // per actual content change, at a fixed kDesignResolution -- never
    // in paintEvent, and never keyed off this widget's own size. Resizing
    // the window (dragging it, moving it to a different projector,
    // shrinking it to a preview thumbnail) triggers Qt resize/paint
    // events, NOT this slot, so it can never cause a re-render.
    const Slide *slide = (m_source == Source::Live) ? m_model->currentSlide()
                                                      : m_model->previewSlide();
    m_canvas = SlideRenderer::render(slide, kDesignResolution);
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

    painter.drawPixmap(targetRect, m_canvas);
}
