#ifndef SANCTIFYLIVE_CORE_IMAGEFRAMINGDIALOG_H_
#define SANCTIFYLIVE_CORE_IMAGEFRAMINGDIALOG_H_

#include <QDialog>
#include <QPixmap>
#include <QPointF>
#include <QWidget>

// Answers the question "which part of this photo survives being cropped
// to fit a screen shaped differently than it is?" -- opened via
// "Edit Framing..." on an image in the Media Library.
//
// A cover-fit crop (see OutputWindow.cpp / SlideView.java) always keeps
// the SAME normalized focus point centered no matter what shape the
// destination screen turns out to be; what changes per-device is only
// how much gets cropped away, not where the crop is anchored. So rather
// than asking the operator to pick a resolution, this dialog just shows
// the image with two representative crop guides overlaid at once --
// a landscape "Projector / TV" (16:9) box and a portrait "Phone" (9:16)
// box -- both driven by one draggable focus point, so it's obvious at a
// glance whether the subject survives either shape before committing.
class ImageFramingDialog : public QDialog
{
    Q_OBJECT

public:
    // initialFocus and the result of focus() are both normalized (0..1
    // across the image's actual width/height), matching
    // Slide::backgroundFocus exactly -- this dialog is only ever used to
    // read and write that one value.
    explicit ImageFramingDialog(const QString &imagePath, const QPointF &initialFocus,
                                 QWidget *parent = nullptr);

    QPointF focus() const;

private:
    class PreviewWidget;
    PreviewWidget *m_preview;
};

#endif // SANCTIFYLIVE_CORE_IMAGEFRAMINGDIALOG_H_
