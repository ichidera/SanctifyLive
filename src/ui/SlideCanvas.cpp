#include "ui/SlideCanvas.h"

#include <QPainter>
#include <QPaintEvent>

SlideCanvas::SlideCanvas(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(160, 90);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);
}

void SlideCanvas::setPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    update();
}

void SlideCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(rect(), Qt::black);

    if (m_pixmap.isNull())
        return;

    const QSize scaledSize = m_pixmap.size().scaled(size(), Qt::KeepAspectRatio);
    QRect targetRect(QPoint(0, 0), scaledSize);
    targetRect.moveCenter(rect().center());

    painter.drawPixmap(targetRect, m_pixmap);
}
