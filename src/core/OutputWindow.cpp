#include "OutputWindow.h"

#include <QPainter>
#include <QPaintEvent>

#include "ScheduleModel.h"

OutputWindow::OutputWindow(ScheduleModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setWindowTitle(tr("SanctifyLive - Output"));
    setMinimumSize(480, 270); // 16:9 floor so text layout stays sane while resizing

    // Plain black until told otherwise -- an output window should never
    // show stale or garbage content before the model has spoken.
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    connect(m_model, &ScheduleModel::liveContentChanged,
            this, &OutputWindow::onLiveContentChanged);
}

void OutputWindow::onLiveContentChanged()
{
    update(); // schedule a repaint; Qt coalesces repeated calls automatically
}

void OutputWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const Slide *slide = m_model->currentSlide();

    // Blackout or empty schedule: fill black and draw nothing else.
    // This is the one-click "instant cut to black" behavior from the
    // feature reference -- it must be unconditional and never show
    // leftover text underneath.
    if (!slide) {
        painter.fillRect(rect(), Qt::black);
        return;
    }

    painter.fillRect(rect(), slide->background);

    if (slide->text.isEmpty())
        return;

    QFont font = painter.font();
    // Scale text to the window height so it stays legible whether this
    // is a small preview or a full-screen projector output.
    font.setPixelSize(qMax(12, height() / 8));
    font.setBold(true);
    painter.setFont(font);

    painter.setPen(Qt::white);
    QRect textRect = rect().adjusted(40, 40, -40, -40);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, slide->text);
}
