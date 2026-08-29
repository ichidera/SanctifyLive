#include "ImageFramingDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace {
constexpr qreal kLandscapeAspect = 16.0 / 9.0;  // "Projector / TV"
constexpr qreal kPortraitAspect  = 9.0 / 16.0;  // "Phone"

// Given the full image size and a target crop aspect ratio, returns the
// largest crop rect of that aspect that fits inside the image, centered
// on centerFraction (0..1 across the image), clamped so it never runs
// off the edge. This is the exact same "cover-fit" sizing/clamping math
// as OutputWindow.cpp and SlideView.java use at render time -- this
// dialog is a preview of that, not a separate cropping tool, so the two
// need to agree.
QRectF coverFitCropRect(const QSizeF &imageSize, qreal targetAspect, const QPointF &centerFraction)
{
    const qreal imageAspect = imageSize.width() / imageSize.height();

    qreal cropW, cropH;
    if (imageAspect > targetAspect) {
        cropH = imageSize.height();
        cropW = cropH * targetAspect;
    } else {
        cropW = imageSize.width();
        cropH = cropW / targetAspect;
    }

    const qreal maxX = qMax<qreal>(0, imageSize.width() - cropW);
    const qreal maxY = qMax<qreal>(0, imageSize.height() - cropH);
    const qreal x = qBound<qreal>(0, centerFraction.x() * imageSize.width() - cropW / 2.0, maxX);
    const qreal y = qBound<qreal>(0, centerFraction.y() * imageSize.height() - cropH / 2.0, maxY);

    return QRectF(x, y, cropW, cropH);
}
}

// The image itself, drawn letterboxed (never cropped) so the whole
// source is always visible, with the two crop guides and a draggable
// focus dot painted on top. All mouse interaction happens in widget
// coordinates and is converted to/from normalized image-space focus.
class ImageFramingDialog::PreviewWidget : public QWidget
{
public:
    explicit PreviewWidget(const QPixmap &image, const QPointF &initialFocus, QWidget *parent = nullptr)
        : QWidget(parent), m_image(image), m_focus(initialFocus)
    {
        setMinimumSize(420, 280);
        setMouseTracking(false);
        setCursor(Qt::CrossCursor);
    }

    QPointF focus() const { return m_focus; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor("#0f0f11"));

        if (m_image.isNull()) {
            painter.setPen(Qt::white);
            painter.drawText(rect(), Qt::AlignCenter, tr("Image failed to load"));
            return;
        }

        m_imageRect = letterboxRect();
        painter.drawPixmap(m_imageRect, m_image, m_image.rect());

        drawGuide(painter, kLandscapeAspect, QColor("#4fa8e0"), tr("Projector / TV  16:9"));
        drawGuide(painter, kPortraitAspect, QColor("#e0a34f"), tr("Phone  9:16"));

        // The focus point itself, so it's clear what's actually being
        // dragged rather than just showing its downstream effect.
        const QPointF dot = imageToWidget(QPointF(m_focus.x() * m_image.width(),
                                                    m_focus.y() * m_image.height()));
        painter.setPen(QPen(Qt::white, 2));
        painter.setBrush(QColor(255, 255, 255, 160));
        painter.drawEllipse(dot, 6, 6);
    }

    void mousePressEvent(QMouseEvent *event) override { updateFocusFromWidgetPos(event->pos()); }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton)
            updateFocusFromWidgetPos(event->pos());
    }

private:
    QRectF letterboxRect() const
    {
        const QSizeF fitted = m_image.size().scaled(size(), Qt::KeepAspectRatio);
        const qreal x = (width() - fitted.width()) / 2.0;
        const qreal y = (height() - fitted.height()) / 2.0;
        return QRectF(QPointF(x, y), fitted);
    }

    QPointF imageToWidget(const QPointF &imagePoint) const
    {
        const qreal scale = m_imageRect.width() / m_image.width();
        return m_imageRect.topLeft() + imagePoint * scale;
    }

    void drawGuide(QPainter &painter, qreal aspect, const QColor &color, const QString &label)
    {
        const QRectF cropInImageSpace = coverFitCropRect(m_image.size(), aspect, m_focus);
        const qreal scale = m_imageRect.width() / m_image.width();
        const QRectF guideRect(m_imageRect.topLeft() + cropInImageSpace.topLeft() * scale,
                                cropInImageSpace.size() * scale);

        QPen pen(color, 2, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(guideRect);

        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        const QFontMetrics fm(painter.font());
        const QRectF labelRect(guideRect.left(), guideRect.top() - fm.height() - 2,
                                fm.horizontalAdvance(label) + 8, fm.height() + 2);
        painter.drawRect(labelRect);
        painter.setPen(Qt::black);
        painter.drawText(labelRect, Qt::AlignCenter, label);
    }

    void updateFocusFromWidgetPos(const QPoint &pos)
    {
        if (m_imageRect.isEmpty())
            return;
        const qreal fx = (pos.x() - m_imageRect.left()) / m_imageRect.width();
        const qreal fy = (pos.y() - m_imageRect.top()) / m_imageRect.height();
        m_focus = QPointF(qBound(0.0, fx, 1.0), qBound(0.0, fy, 1.0));
        update();
    }

    QPixmap m_image;
    QPointF m_focus;
    QRectF m_imageRect; // where the (letterboxed) image was last painted -- for hit-testing
};

ImageFramingDialog::ImageFramingDialog(const QString &imagePath, const QPointF &initialFocus,
                                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Framing"));
    resize(560, 420);

    const QPixmap image(imagePath);
    m_preview = new PreviewWidget(image, initialFocus, this);

    auto *hint = new QLabel(
        tr("Drag the dot to keep the important part of the photo in frame, no matter"
           " which shape screen it ends up on."),
        this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #aaaaaa;");

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_preview, 1);
    layout->addWidget(hint);
    layout->addWidget(buttons);
}

QPointF ImageFramingDialog::focus() const
{
    return m_preview->focus();
}
