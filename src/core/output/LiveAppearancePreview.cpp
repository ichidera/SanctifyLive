#include "LiveAppearancePreview.h"

#include <QPainter>
#include <QPaintEvent>

#include "SlideRenderer.h"

LiveAppearancePreview::LiveAppearancePreview(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(160, 90); // 16:9 floor, matches OutputWindow's own minimum ratio
}

QSize LiveAppearancePreview::sizeHint() const
{
    return {320, 180};
}

QSize LiveAppearancePreview::minimumSizeHint() const
{
    return {160, 90};
}

void LiveAppearancePreview::setSlide(const Slide &slide)
{
    m_slide = slide;
    m_hasSlide = true;
    update();
}

void LiveAppearancePreview::clearSlide()
{
    m_hasSlide = false;
    update();
}

void LiveAppearancePreview::setProfile(const OutputProfile &profile)
{
    m_profile = profile;
    update();
}

QRect LiveAppearancePreview::frameRect() const
{
    return SlideRenderer::fittedFrame(rect().adjusted(2, 2, -2, -2), m_profile.outputPosition.size());
}

void LiveAppearancePreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // The area outside the aspect-correct frame is this widget's own
    // background, not part of the output -- kept visually distinct from
    // the frame itself (which is always pure black-or-content, exactly
    // like the real screen) so it reads as "letterboxing," not as part
    // of what will be projected.
    painter.fillRect(rect(), QColor("#1a1a1c"));

    const QRect frame = frameRect();

    if (m_hasSlide) {
        SlideRenderer::paint(painter, frame, m_slide, m_profile);
    } else {
        painter.fillRect(frame, Qt::black);
        painter.setPen(QColor("#55555a"));
        QFont placeholderFont = painter.font();
        placeholderFont.setPixelSize(qMax(11, frame.height() / 12));
        painter.setFont(placeholderFont);
        painter.drawText(frame, Qt::AlignCenter | Qt::TextWordWrap, tr("Nothing selected"));
    }

    painter.setPen(QPen(QColor("#3a3a3d"), 1));
    painter.drawRect(frame.adjusted(0, 0, -1, -1));

    if (!m_profile.outputPosition.isEmpty()) {
        const QString label = QStringLiteral("%1\u00d7%2")
                                   .arg(m_profile.outputPosition.width())
                                   .arg(m_profile.outputPosition.height());
        SlideRenderer::drawResolutionBadge(painter, frame, label);
    }
}
