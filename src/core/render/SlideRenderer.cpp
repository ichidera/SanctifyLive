#include "core/render/SlideRenderer.h"

#include <QPainter>

#include "core/model/Slide.h"

namespace SlideRenderer
{

QPixmap render(const Slide *slide, const QSize &resolution)
{
    QPixmap canvas(resolution);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);

    // Blackout or empty schedule: fill black and draw nothing else. This
    // is the one-click "instant cut to black" behavior from the feature
    // reference -- it must be unconditional and never show leftover text
    // underneath.
    if (!slide) {
        canvas.fill(Qt::black);
        return canvas;
    }

    canvas.fill(slide->background);

    if (slide->text.isEmpty())
        return canvas;

    // Font size and margins are proportions of the DESIGN resolution,
    // never of a physical widget size -- that's what makes the frame
    // identical on a 1080p monitor, a 720p projector, or a small preview
    // thumbnail. The only thing that differs between those is how the
    // resulting pixmap gets scaled afterwards (see OutputWindow).
    QFont font = painter.font();
    font.setPixelSize(qMax(12, resolution.height() / 8));
    font.setBold(true);
    painter.setFont(font);

    painter.setPen(Qt::white);
    const int marginX = resolution.width() / 24;
    const int marginY = resolution.height() / 24;
    QRect textRect = canvas.rect().adjusted(marginX, marginY, -marginX, -marginY);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, slide->text);

    return canvas;
}

} // namespace SlideRenderer
