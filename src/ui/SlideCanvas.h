#pragma once

#include <QPixmap>
#include <QWidget>

// SlideCanvas displays a single pre-rendered QPixmap, scaled and
// letterboxed to fit whatever physical size this widget has -- the same
// "render once, scale everywhere" contract OutputWindow follows (see
// RenderResolution.h). It does not know about ScheduleModel or slides
// at all; callers hand it a pixmap via setPixmap().
//
// This exists so any panel that needs to show a rendered slide/swatch
// without being "the" live output can share OutputWindow's exact
// scaling behavior instead of duplicating paint logic inline. Two
// panels in OperatorWindow's glossary use this directly: PREVIEW
// (m_previewCanvas, whichever Schedule row is selected) and ITEM
// PREVIEW (m_itemPreviewCanvas, whichever Content Tab item is
// selected/hovered, e.g. in the Media tab). LIVE uses an embedded
// OutputWindow instead, since it needs the real congregation-facing
// class's behavior, not just its rendering trick.
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
