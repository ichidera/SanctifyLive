#ifndef SANCTIFYLIVE_CORE_ICONFACTORY_H_
#define SANCTIFYLIVE_CORE_ICONFACTORY_H_


#include <QIcon>
#include <QPainter>
#include <QPixmap>

// Small, original, flat-style icons drawn with QPainter -- not copied
// from any application's actual icon assets. These exist purely so the
// toolbar/tree don't ship with blank buttons; swapping in a real icon
// set later only touches this file.
namespace IconFactory
{
inline QIcon draw(int size, const std::function<void(QPainter &, int)> &paint)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    paint(painter, size);
    return QIcon(pixmap);
}

inline QIcon newDocument(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#dcdcdc"), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRect(n * 0.28, n * 0.12, n * 0.44, n * 0.76);
        p.setPen(QPen(QColor("#2ecc71"), 2));
        p.drawLine(n * 0.5, n * 0.42, n * 0.5, n * 0.68);
        p.drawLine(n * 0.37, n * 0.55, n * 0.63, n * 0.55);
    });
}

inline QIcon openFolder(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#dcdcdc"), 1.5));
        p.setBrush(QColor("#f1c40f"));
        p.drawRect(n * 0.15, n * 0.35, n * 0.7, n * 0.42);
        p.drawRect(n * 0.15, n * 0.24, n * 0.32, n * 0.14);
    });
}

inline QIcon save(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#dcdcdc"), 1.5));
        p.setBrush(QColor("#5dade2"));
        p.drawRoundedRect(n * 0.18, n * 0.14, n * 0.64, n * 0.72, 3, 3);
        p.setBrush(QColor("#1c1c1f"));
        p.drawRect(n * 0.3, n * 0.16, n * 0.4, n * 0.24);
    });
}

inline QIcon store(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#a97ad1"), 1.8));
        p.setBrush(Qt::NoBrush);
        QPolygonF bag;
        bag << QPointF(n * 0.22, n * 0.32) << QPointF(n * 0.78, n * 0.32)
            << QPointF(n * 0.72, n * 0.86) << QPointF(n * 0.28, n * 0.86);
        p.drawPolygon(bag);
        p.drawArc(n * 0.34, n * 0.12, n * 0.32, n * 0.32, 0, 180 * 16);
    });
}

inline QIcon web(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#5dade2"), 1.6));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(n * 0.15, n * 0.15, n * 0.7, n * 0.7));
        p.drawEllipse(QRectF(n * 0.32, n * 0.15, n * 0.36, n * 0.7));
        p.drawLine(QPointF(n * 0.16, n * 0.5), QPointF(n * 0.84, n * 0.5));
    });
}

inline QIcon remote(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#5dade2"), 2));
        p.setBrush(QColor("#5dade2"));
        p.drawEllipse(QRectF(n * 0.44, n * 0.44, n * 0.12, n * 0.12));
        p.setBrush(Qt::NoBrush);
        p.drawArc(n * 0.3, n * 0.3, n * 0.4, n * 0.4, 45 * 16, 90 * 16);
        p.drawArc(n * 0.16, n * 0.16, n * 0.68, n * 0.68, 45 * 16, 90 * 16);
    });
}

inline QIcon goLive(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#27ae60"));
        p.drawEllipse(QRectF(n * 0.1, n * 0.1, n * 0.8, n * 0.8));
        p.setBrush(Qt::white);
        QPolygonF tri;
        tri << QPointF(n * 0.4, n * 0.32) << QPointF(n * 0.4, n * 0.68) << QPointF(n * 0.68, n * 0.5);
        p.drawPolygon(tri);
    });
}

inline QIcon alerts(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#f1c40f"), 1.6));
        p.setBrush(QColor("#f1c40f"));
        p.drawChord(n * 0.28, n * 0.2, n * 0.44, n * 0.5, 0, 180 * 16);
        p.setBrush(Qt::NoBrush);
        p.drawArc(n * 0.22, n * 0.16, n * 0.56, n * 0.48, 0, 180 * 16);
        p.setBrush(QColor("#f1c40f"));
        p.drawEllipse(QRectF(n * 0.44, n * 0.68, n * 0.12, n * 0.12));
    });
}

inline QIcon logo(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#dcdcdc"), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRect(n * 0.15, n * 0.2, n * 0.7, n * 0.56);
        p.setBrush(QColor("#e67e22"));
        p.setPen(Qt::NoPen);
        QPolygonF mountain;
        mountain << QPointF(n * 0.2, n * 0.7) << QPointF(n * 0.42, n * 0.42)
                 << QPointF(n * 0.58, n * 0.58) << QPointF(n * 0.72, n * 0.38)
                 << QPointF(n * 0.8, n * 0.7);
        p.drawPolygon(mountain);
        p.setBrush(QColor("#f1c40f"));
        p.drawEllipse(QRectF(n * 0.6, n * 0.28, n * 0.1, n * 0.1));
    });
}

inline QIcon blackScreen(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#888888"), 1.4));
        p.setBrush(QColor("#111111"));
        p.drawRoundedRect(QRectF(n * 0.14, n * 0.2, n * 0.72, n * 0.5), 2, 2);
        p.drawRect(QRectF(n * 0.42, n * 0.72, n * 0.16, n * 0.08));
    });
}

inline QIcon clearScreen(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#888888"), 1.4));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(n * 0.14, n * 0.2, n * 0.72, n * 0.5), 2, 2);
        p.setPen(QPen(QColor("#e74c3c"), 2));
        p.drawLine(QPointF(n * 0.18, n * 0.24), QPointF(n * 0.82, n * 0.66));
    });
}

inline QIcon liveMonitor(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#e74c3c"), 1.6));
        p.setBrush(QColor("#2b2b2e"));
        p.drawRoundedRect(QRectF(n * 0.12, n * 0.16, n * 0.76, n * 0.5), 2, 2);
        p.setBrush(QColor("#e74c3c"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(n * 0.42, n * 0.3, n * 0.16, n * 0.16));
        p.setPen(QPen(QColor("#888888"), 1.4));
        p.drawLine(QPointF(n * 0.5, n * 0.66), QPointF(n * 0.5, n * 0.78));
        p.drawLine(QPointF(n * 0.34, n * 0.8), QPointF(n * 0.66, n * 0.8));
    });
}

// A plain output-monitor glyph, parameterized by accent color so the
// Options sidebar can give Main Output/Alternate Output/Foldback a
// visually distinct (but clearly related) icon without three near-copies
// of this function. Deliberately calmer than liveMonitor() above, which
// is reserved for the toolbar's "Live" action.
inline QIcon monitorOutput(const QColor &accent, int s = 20)
{
    return draw(s, [accent](QPainter &p, int n) {
        p.setPen(QPen(QColor("#9a9a9e"), 1.4));
        p.setBrush(QColor("#2b2b2e"));
        p.drawRoundedRect(QRectF(n * 0.08, n * 0.14, n * 0.84, n * 0.56), 2, 2);
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawRoundedRect(QRectF(n * 0.18, n * 0.24, n * 0.64, n * 0.36), 1, 1);
        p.setPen(QPen(QColor("#9a9a9e"), 1.4));
        p.drawLine(QPointF(n * 0.5, n * 0.7), QPointF(n * 0.5, n * 0.82));
        p.drawLine(QPointF(n * 0.32, n * 0.86), QPointF(n * 0.68, n * 0.86));
    });
}

inline QIcon androidRobot(int s = 20)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#a4c639"));
        p.drawRoundedRect(QRectF(n * 0.2, n * 0.36, n * 0.6, n * 0.44), n * 0.08, n * 0.08);
        p.drawRect(QRectF(n * 0.08, n * 0.4, n * 0.1, n * 0.3));
        p.drawRect(QRectF(n * 0.82, n * 0.4, n * 0.1, n * 0.3));
        p.drawRect(QRectF(n * 0.3, n * 0.78, n * 0.12, n * 0.16));
        p.drawRect(QRectF(n * 0.58, n * 0.78, n * 0.12, n * 0.16));
        p.drawArc(QRectF(n * 0.22, n * 0.06, n * 0.56, n * 0.56), 0, 180 * 16);
        p.setPen(QPen(Qt::white, n * 0.05));
        p.drawLine(QPointF(n * 0.38, n * 0.5), QPointF(n * 0.38, n * 0.6));
        p.drawLine(QPointF(n * 0.62, n * 0.5), QPointF(n * 0.62, n * 0.6));
    });
}

inline QIcon clock(int s = 20)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#5dade2"), 1.6));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(n * 0.1, n * 0.1, n * 0.8, n * 0.8));
        p.drawLine(QPointF(n * 0.5, n * 0.5), QPointF(n * 0.5, n * 0.26));
        p.drawLine(QPointF(n * 0.5, n * 0.5), QPointF(n * 0.68, n * 0.58));
    });
}

inline QIcon gear(int s = 20)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#9a9a9e"));
        const double cx = n * 0.5, cy = n * 0.5, rOuter = n * 0.42, rInner = n * 0.3, toothR = n * 0.08;
        for (int i = 0; i < 8; ++i) {
            const double angle = i * M_PI / 4;
            p.drawEllipse(QPointF(cx + rOuter * std::cos(angle), cy + rOuter * std::sin(angle)), toothR, toothR);
        }
        p.drawEllipse(QPointF(cx, cy), rInner, rInner);
        p.setBrush(QColor("#1c1c1f"));
        p.drawEllipse(QPointF(cx, cy), rInner * 0.45, rInner * 0.45);
    });
}

inline QIcon search(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#aaaaaa"), 1.6));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(n * 0.15, n * 0.15, n * 0.5, n * 0.5));
        p.drawLine(QPointF(n * 0.58, n * 0.58), QPointF(n * 0.85, n * 0.85));
    });
}

// --- Small tree-view glyphs (16px) ---

inline QIcon treeVideo(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#5dade2"), 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(n * 0.1, n * 0.28, n * 0.55, n * 0.44));
        p.setBrush(QColor("#5dade2"));
        p.setPen(Qt::NoPen);
        QPolygonF tri;
        tri << QPointF(n * 0.68, n * 0.36) << QPointF(n * 0.68, n * 0.64) << QPointF(n * 0.92, n * 0.5);
        p.drawPolygon(tri);
    });
}

inline QIcon treeImage(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#e67e22"), 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(n * 0.1, n * 0.2, n * 0.8, n * 0.6));
        p.setBrush(QColor("#e67e22"));
        p.setPen(Qt::NoPen);
        QPolygonF mountain;
        mountain << QPointF(n * 0.15, n * 0.75) << QPointF(n * 0.35, n * 0.45)
                 << QPointF(n * 0.55, n * 0.65) << QPointF(n * 0.7, n * 0.4)
                 << QPointF(n * 0.85, n * 0.75);
        p.drawPolygon(mountain);
    });
}

inline QIcon treeFeed(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#f39c12"), 1.4));
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor("#f39c12"), 1.6));
        p.drawArc(n * 0.15, n * 0.15, n * 0.7, n * 0.7, 45 * 16, 90 * 16);
        p.drawArc(n * 0.15, n * 0.15, n * 0.45, n * 0.45, 45 * 16, 90 * 16);
        p.setBrush(QColor("#f39c12"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(n * 0.15, n * 0.62, n * 0.2, n * 0.2));
    });
}

inline QIcon treeDvd(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#95a5a6"), 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(n * 0.1, n * 0.1, n * 0.8, n * 0.8));
        p.setBrush(QColor("#95a5a6"));
        p.drawEllipse(QRectF(n * 0.42, n * 0.42, n * 0.16, n * 0.16));
    });
}

inline QIcon treeAudio(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#9b59b6"), 1.2));
        p.setBrush(QColor("#9b59b6"));
        QPolygonF speaker;
        speaker << QPointF(n * 0.15, n * 0.38) << QPointF(n * 0.35, n * 0.38)
                << QPointF(n * 0.55, n * 0.2) << QPointF(n * 0.55, n * 0.8)
                << QPointF(n * 0.35, n * 0.62) << QPointF(n * 0.15, n * 0.62);
        p.drawPolygon(speaker);
        p.setBrush(Qt::NoBrush);
        p.drawArc(n * 0.62, n * 0.3, n * 0.28, n * 0.4, -60 * 16, 120 * 16);
    });
}

inline QIcon treePremium(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f1c40f"));
        QPolygonF star;
        const double cx = n * 0.5, cy = n * 0.5, rOuter = n * 0.45, rInner = n * 0.19;
        for (int i = 0; i < 10; ++i) {
            const double r = (i % 2 == 0) ? rOuter : rInner;
            const double angle = -M_PI / 2 + i * M_PI / 5;
            star << QPointF(cx + r * std::cos(angle), cy + r * std::sin(angle));
        }
        p.drawPolygon(star);
    });
}

inline QIcon hamburgerMenu(int s = 16)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#aaaaaa"), 1.6));
        p.drawLine(QPointF(n * 0.16, n * 0.28), QPointF(n * 0.84, n * 0.28));
        p.drawLine(QPointF(n * 0.16, n * 0.5), QPointF(n * 0.84, n * 0.5));
        p.drawLine(QPointF(n * 0.16, n * 0.72), QPointF(n * 0.84, n * 0.72));
    });
}

inline QIcon diskImport(int s = 28)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#dcdcdc"), 1.5));
        p.setBrush(QColor("#5dade2"));
        p.drawEllipse(QRectF(n * 0.12, n * 0.12, n * 0.76, n * 0.76));
        p.setPen(QPen(QColor("#1c1c1f"), 2));
        p.drawLine(QPointF(n * 0.5, n * 0.3), QPointF(n * 0.5, n * 0.58));
        p.drawLine(QPointF(n * 0.36, n * 0.46), QPointF(n * 0.5, n * 0.6));
        p.drawLine(QPointF(n * 0.64, n * 0.46), QPointF(n * 0.5, n * 0.6));
        p.drawLine(QPointF(n * 0.32, n * 0.7), QPointF(n * 0.68, n * 0.7));
    });
}

inline QIcon plusAdd(int s = 20)
{
    return draw(s, [](QPainter &p, int n) {
        p.setPen(QPen(QColor("#dcdcdc"), 2));
        p.drawLine(QPointF(n * 0.5, n * 0.18), QPointF(n * 0.5, n * 0.82));
        p.drawLine(QPointF(n * 0.18, n * 0.5), QPointF(n * 0.82, n * 0.5));
    });
}
}

#endif // SANCTIFYLIVE_CORE_ICONFACTORY_H_