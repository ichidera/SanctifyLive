#pragma once

#include <QPixmap>
#include <QWidget>

// SlideCanvas displays a single pre-rendered QPixmap, scaled and
// letterboxed to fit whatever physical size this widget has -- the same
// "render once, scale everywhere" contract OutputWindow follows (see
// RenderResolution.h). It does not know about ScheduleModel or slides
// at all; callers hand it a pixmap via setPixmap().
//
// This exists so OperatorWindow's "Preview" pane (which shows whatever
// slide is currently *selected*, not necessarily what's live) can share
// the exact same scaling behavior as the real Live/output pane without
// OperatorWindow reaching into OutputWindow's internals or duplicating
// paint logic inline.
class SlideCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit SlideCanvas(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_pixmap;
};
