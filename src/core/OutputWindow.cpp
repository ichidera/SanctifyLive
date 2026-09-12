#include "OutputWindow.h"

#include <QPainter>
#include <QPaintEvent>

#include "ScheduleModel.h"

OutputWindow::OutputWindow(ScheduleModel *model, Source source, QWidget *parent)
    : QWidget(parent), m_model(model), m_source(source)
{
    setWindowTitle(tr("SanctifyLive - Output"));
    setMinimumSize(320, 180); // 16:9 floor so text layout stays sane while resizing

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    if (m_source == Source::Live) {
        connect(m_model, &ScheduleModel::liveContentChanged,
                this, &OutputWindow::onContentChanged);
    } else {
        connect(m_model, &ScheduleModel::previewChanged,
                this, &OutputWindow::onContentChanged);
    }
    // Either pane also needs to repaint if a slide's own content changed
    // (e.g. edited in place) or the schedule shrank out from under it.
    connect(m_model, &ScheduleModel::scheduleChanged, this, &OutputWindow::onContentChanged);
}

void OutputWindow::onContentChanged()
{
    update();
}

void OutputWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Slide *slide = (m_source == Source::Live) ? m_model->liveSlide() : m_model->previewSlide();

    // No slide (blackout, in Live mode, or an empty schedule): fill black
    // and draw nothing else. Must be unconditional -- this is the
    // "instant cut to black" behavior and it must never show stale text.
    if (!slide) {
        painter.fillRect(rect(), Qt::black);
        return;
    }

    painter.fillRect(rect(), slide->background);

    if (slide->text.isEmpty())
        return;

    QFont font = painter.font();
    font.setPixelSize(qMax(12, height() / 8));
    font.setBold(true);
    painter.setFont(font);

    painter.setPen(Qt::white);
    QRect textRect = rect().adjusted(40, 40, -40, -40);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, slide->text);
}