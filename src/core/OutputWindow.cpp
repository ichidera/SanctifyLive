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

    if (!slide->backgroundImagePath.isEmpty()) {
        const QPixmap pixmap(slide->backgroundImagePath);
        if (!pixmap.isNull()) {
            // Cover-fit: scale to fill the widget, cropping any overflow,
            // same idea as CSS background-size: cover. The crop window is
            // biased toward the slide's focus point (default dead-center,
            // reproducing the old behavior exactly) rather than always
            // centering, so an operator can keep a photo's actual subject
            // in frame even when this window's aspect ratio doesn't match
            // the source image's -- see Slide::backgroundFocus.
            const QPixmap scaled = pixmap.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
            const int maxX = qMax(0, scaled.width() - width());
            const int maxY = qMax(0, scaled.height() - height());
            const int x = qBound(0, qRound(slide->backgroundFocus.x() * scaled.width()
                                            - width() / 2.0), maxX);
            const int y = qBound(0, qRound(slide->backgroundFocus.y() * scaled.height()
                                            - height() / 2.0), maxY);
            const QRect sourceRect(x, y, width(), height());
            painter.drawPixmap(rect(), scaled, sourceRect);
        } else {
            // File went missing/unreadable since it was imported -- fail
            // back to the solid color rather than showing nothing.
            painter.fillRect(rect(), slide->background);
        }
    } else {
        painter.fillRect(rect(), slide->background);
    }

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