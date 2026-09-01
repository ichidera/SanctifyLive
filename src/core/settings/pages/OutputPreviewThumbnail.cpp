#include "OutputPreviewThumbnail.h"

#include <QPainter>

#include "../../output/SlideRenderer.h"
#include "../../schedule/Slide.h"

OutputPreviewThumbnail::OutputPreviewThumbnail(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(96);
}

QSize OutputPreviewThumbnail::sizeHint() const
{
    return {188, 106};
}

void OutputPreviewThumbnail::setProfile(const OutputProfile *profile)
{
    m_hasProfile = (profile != nullptr);
    if (profile)
        m_profile = *profile;
    m_enabledLook = profile && profile->monitorIndex >= 0;
    update();
}

QRect OutputPreviewThumbnail::frameRect() const
{
    const QRect available = rect().adjusted(1, 1, -1, -1);
    if (!m_hasProfile)
        return available;
    return SlideRenderer::fittedFrame(available, m_profile.outputPosition.size());
}

void OutputPreviewThumbnail::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(QColor(m_enabledLook ? "#3a3a3d" : "#2a2a2c"), 1));
    p.setBrush(QColor(m_enabledLook ? "#0e0e10" : "#161617"));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 3, 3);

    if (!m_hasProfile)
        return;

    const QRect frame = frameRect();

    Slide sample(tr("Preview"), m_sampleLines.join(QStringLiteral("\n")), Qt::black);
    SlideRenderer::paint(p, frame, sample, m_profile);

    if (!m_enabledLook) {
        // Dim the whole rendered frame rather than skipping the render,
        // so an unassigned output still previews *what* would go out
        // (font, margins, aspect ratio) -- just visibly inactive.
        p.fillRect(frame, QColor(10, 10, 11, 150));
    }

    p.setPen(QPen(QColor(m_enabledLook ? "#3a3a3d" : "#2a2a2c"), 1));
    p.drawRect(frame.adjusted(0, 0, -1, -1));

    if (m_enabledLook) {
        const QString label = QStringLiteral("%1\u00d7%2")
                                   .arg(m_profile.outputPosition.width())
                                   .arg(m_profile.outputPosition.height());
        SlideRenderer::drawResolutionBadge(p, frame, label);
    }
}
