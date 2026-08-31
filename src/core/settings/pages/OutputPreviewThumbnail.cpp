#include "OutputPreviewThumbnail.h"

#include <QPainter>

OutputPreviewThumbnail::OutputPreviewThumbnail(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(96);
}

QSize OutputPreviewThumbnail::sizeHint() const
{
    return {188, 106};
}

void OutputPreviewThumbnail::setSampleLines(const QStringList &lines)
{
    m_sampleLines = lines;
    update();
}

void OutputPreviewThumbnail::setResolutionLabel(const QString &label)
{
    m_resolutionLabel = label;
    update();
}

void OutputPreviewThumbnail::setEnabledLook(bool enabled)
{
    m_enabledLook = enabled;
    update();
}

void OutputPreviewThumbnail::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF frame = rect().adjusted(1, 1, -1, -1);
    p.setPen(QPen(QColor(m_enabledLook ? "#3a3a3d" : "#2a2a2c"), 1));
    p.setBrush(QColor(m_enabledLook ? "#0e0e10" : "#161617"));
    p.drawRoundedRect(frame, 3, 3);

    if (!m_resolutionLabel.isEmpty()) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#c0392b"));
        const QRectF badge(frame.right() - 46, frame.top() + 4, 42, 16);
        p.drawRoundedRect(badge, 3, 3);
        p.setPen(Qt::white);
        QFont badgeFont = font();
        badgeFont.setPointSizeF(badgeFont.pointSizeF() * 0.75);
        p.setFont(badgeFont);
        p.drawText(badge, Qt::AlignCenter, m_resolutionLabel);
    }

    p.setPen(m_enabledLook ? QColor("#e8e8ea") : QColor("#55555a"));
    QFont sampleFont = font();
    sampleFont.setPointSizeF(sampleFont.pointSizeF() * 0.85);
    p.setFont(sampleFont);

    const qreal lineHeight = frame.height() / qMax(1, m_sampleLines.size() + 1);
    qreal y = frame.top() + lineHeight * 0.6;
    for (const QString &line : m_sampleLines) {
        p.drawText(QRectF(frame.left(), y, frame.width(), lineHeight), Qt::AlignHCenter, line);
        y += lineHeight;
    }
}
