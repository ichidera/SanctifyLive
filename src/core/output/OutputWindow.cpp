#include "OutputWindow.h"

#include <QPainter>
#include <QPaintEvent>

#include "../schedule/ScheduleModel.h"
#include "SlideRenderer.h"

OutputWindow::OutputWindow(ScheduleModel *model, Source source, QWidget *parent)
    : QWidget(parent), m_model(model), m_source(source)
{
    setWindowTitle(tr("SanctifyLive - Output"));
    setMinimumSize(320, 180); // 16:9 floor so text layout stays sane while resizing

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    // Both Live and Preview modes are driven by the same underlying live
    // content now (see ScheduleModel) -- the only difference between them
    // is that Live respects blackout and Preview doesn't. So both listen
    // to the same signal.
    connect(m_model, &ScheduleModel::liveContentChanged, this, &OutputWindow::onContentChanged);
    connect(m_model, &ScheduleModel::scheduleChanged, this, &OutputWindow::onContentChanged);
}

void OutputWindow::onContentChanged()
{
    update();
}

void OutputWindow::setProfile(const OutputProfile &profile)
{
    m_profile = profile;
    update();
}

void OutputWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    const Slide *slide = (m_source == Source::Live) ? m_model->liveSlide() : m_model->previewSlide();

    // No slide (blackout, in Live mode, or an empty schedule): fill black
    // and draw nothing else. Must be unconditional -- this is the
    // "instant cut to black" behavior and it must never show stale text.
    if (!slide) {
        painter.fillRect(rect(), Qt::black);
        return;
    }

    // Same rendering code the in-app previews use (see
    // src/core/output/SlideRenderer.h) -- this window fills the whole
    // widget, so targetRect == rect() means margins/font are drawn at
    // their real configured size, not scaled down the way a small
    // preview panel's frame would be.
    SlideRenderer::paint(painter, rect(), *slide, m_profile);
}