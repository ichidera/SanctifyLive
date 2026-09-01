#include "SlideRenderer.h"

#include <QFont>
#include <QPainter>
#include <QPixmap>

#include "../schedule/Slide.h"
#include "../settings/OutputProfile.h"

namespace SlideRenderer {

void paint(QPainter &painter, const QRect &targetRect, const Slide &slide, const OutputProfile &profile)
{
    painter.save();
    painter.setClipRect(targetRect);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!slide.backgroundImagePath.isEmpty()) {
        const QPixmap pixmap(slide.backgroundImagePath);
        if (!pixmap.isNull()) {
            // Cover-fit: scale to fill targetRect, cropping any overflow,
            // same idea as CSS background-size: cover, biased toward the
            // slide's focus point -- see Slide::backgroundFocus.
            const QPixmap scaled = pixmap.scaled(targetRect.size(), Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
            const int maxX = qMax(0, scaled.width() - targetRect.width());
            const int maxY = qMax(0, scaled.height() - targetRect.height());
            const int x = qBound(0, qRound(slide.backgroundFocus.x() * scaled.width()
                                            - targetRect.width() / 2.0), maxX);
            const int y = qBound(0, qRound(slide.backgroundFocus.y() * scaled.height()
                                            - targetRect.height() / 2.0), maxY);
            const QRect sourceRect(x, y, targetRect.width(), targetRect.height());
            painter.drawPixmap(targetRect, scaled, sourceRect);
        } else {
            // File went missing/unreadable since it was imported -- fail
            // back to the solid color rather than showing nothing.
            painter.fillRect(targetRect, slide.background);
        }
    } else {
        painter.fillRect(targetRect, slide.background);
    }

    if (slide.text.isEmpty()) {
        painter.restore();
        return;
    }

    // Margins are absolute pixels sized for the destination's real,
    // configured resolution -- scale them (and the font) down/up by
    // however much smaller/larger targetRect is than that, so a preview
    // reproduces the same proportions the real screen would show.
    const int nativeHeight = qMax(1, profile.outputPosition.height());
    const qreal scale = qreal(targetRect.height()) / nativeHeight;

    const QRect textRect = targetRect.adjusted(qRound(profile.marginLeft * scale),
                                                qRound(profile.marginTop * scale),
                                                -qRound(profile.marginRight * scale),
                                                -qRound(profile.marginBottom * scale));

    QFont font = profile.defaultFont.family().isEmpty() ? painter.font() : profile.defaultFont;
    font.setPixelSize(qMax(12, targetRect.height() / 8));
    font.setBold(true);
    painter.setFont(font);

    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, slide.text);
    painter.restore();
}

QRect fittedFrame(const QRect &available, const QSize &nativeResolution)
{
    if (available.isEmpty())
        return available;

    if (nativeResolution.width() <= 0 || nativeResolution.height() <= 0)
        return available; // nothing sensible to fit to -- use the whole area as-is

    const QSize fitted = nativeResolution.scaled(available.size(), Qt::KeepAspectRatio);
    QRect frame(0, 0, qMax(1, fitted.width()), qMax(1, fitted.height()));
    frame.moveCenter(available.center());
    return frame;
}

void drawResolutionBadge(QPainter &painter, const QRect &frame, const QString &label)
{
    if (label.isEmpty())
        return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#c0392b"));
    const QRectF badge(frame.right() - 62, frame.top() + 6, 58, 18);
    painter.drawRoundedRect(badge, 3, 3);

    painter.setPen(Qt::white);
    QFont badgeFont = painter.font();
    badgeFont.setPixelSize(11);
    badgeFont.setBold(true);
    painter.setFont(badgeFont);
    painter.drawText(badge, Qt::AlignCenter, label);
    painter.restore();
}

} // namespace SlideRenderer
