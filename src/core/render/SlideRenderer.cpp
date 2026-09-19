#include "core/render/SlideRenderer.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>

#include "core/model/Slide.h"

namespace SlideRenderer
{

namespace
{

// The largest pixel size (using `baseFont`'s family/weight) at which
// `text`, word-wrapped, fits entirely inside `bounds` -- found by binary
// search rather than any fixed fraction of the canvas. A one-line slide
// title ("Beach Sunset") and an eight-verse combined reading need very
// different sizes to both fill their box without overflowing it, and no
// single formula based on resolution alone can tell which one it's
// looking at without knowing how much text there actually is.
//
// QFontMetrics::boundingRect() runs the same word-wrap layout
// drawText() will use, so "does it fit" here means exactly what it
// means when actually painted -- this can't drift out of sync with the
// real draw call the way a separately-maintained line-counting formula
// could.
int fittedFontPixelSize(const QFont &baseFont, const QString &text, const QRect &bounds)
{
    constexpr int kMinFontPixelSize = 12; // below this, text is unreadable on a projector regardless of fit

    if (bounds.isEmpty() || text.isEmpty())
        return kMinFontPixelSize;

    // A single line can't be taller than the box it has to fit in, so
    // the box's own height is the natural (not arbitrary) upper bound --
    // no magic constant needed here.
    const int maxFontPixelSize = qMax(kMinFontPixelSize, bounds.height());

    auto fits = [&](int pixelSize) {
        QFont candidate = baseFont;
        candidate.setPixelSize(pixelSize);
        const QFontMetrics metrics(candidate);
        const QRect wrapped = metrics.boundingRect(bounds, Qt::TextWordWrap | Qt::AlignCenter, text);
        return wrapped.height() <= bounds.height() && wrapped.width() <= bounds.width();
    };

    // Even the minimum size doesn't fully fit (an exceptionally long
    // multi-verse selection): that's still the most readable option
    // available, so use it rather than shrinking text past legibility
    // to chase a fit that isn't achievable.
    if (!fits(kMinFontPixelSize))
        return kMinFontPixelSize;

    int low = kMinFontPixelSize;
    int high = maxFontPixelSize;
    while (low < high) {
        const int mid = low + (high - low + 1) / 2; // bias upward so this converges on the largest fitting size
        if (fits(mid))
            low = mid;
        else
            high = mid - 1;
    }
    return low;
}

} // namespace

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

    // Real background image, cover-fit (scaled up to fully cover the
    // frame, then center-cropped) so it fills the canvas the same way a
    // photo background would in any slide/presentation tool -- never
    // letterboxed, never stretched out of proportion. Falls back to the
    // solid `background` color if the path is empty or the file can't
    // be decoded, rather than showing a blank/broken frame.
    QPixmap background;
    if (!slide->backgroundImagePath.isEmpty())
        background = QPixmap(slide->backgroundImagePath);

    if (!background.isNull()) {
        const QPixmap scaled = background.scaled(resolution, Qt::KeepAspectRatioByExpanding,
                                                   Qt::SmoothTransformation);
        const int srcX = (scaled.width() - resolution.width()) / 2;
        const int srcY = (scaled.height() - resolution.height()) / 2;
        painter.drawPixmap(0, 0, scaled, srcX, srcY, resolution.width(), resolution.height());
    } else {
        canvas.fill(slide->background);
    }

    if (slide->text.isEmpty())
        return canvas;

    // Margins are a proportion of the DESIGN resolution, never of a
    // physical widget size -- that's what makes the frame identical on
    // a 1080p monitor, a 720p projector, or a small preview thumbnail.
    // The only thing that differs between those is how the resulting
    // pixmap gets scaled afterwards (see OutputWindow).
    const int marginX = resolution.width() / 24;
    const int marginY = resolution.height() / 24;
    const QRect textRect = canvas.rect().adjusted(marginX, marginY, -marginX, -marginY);

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(fittedFontPixelSize(font, slide->text, textRect));
    painter.setFont(font);

    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, slide->text);

    return canvas;
}

} // namespace SlideRenderer
