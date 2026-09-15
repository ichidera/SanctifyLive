#pragma once

#include <QColor>
#include <QString>

// A Slide is the smallest unit of content that can be put on the live
// output: a label (shown in the operator's schedule list, never on
// screen), the text body, a background color, and -- since real image
// import landed in the Media tab -- an optional background image path.
//
// backgroundImagePath is deliberately just a path, not embedded pixel
// data: Slide stays cheap to copy (ScheduleModel copies these around
// freely), and SlideRenderer is the one place that actually loads and
// draws it. When empty, `background` is used as a plain solid fill --
// that's still how the built-in Media swatches and any future themed
// slides work. When non-empty, SlideRenderer draws the real image
// (cover-fit, cropped to the frame) and `background` is only a fallback
// for if the file can't be loaded.
//
// Later milestones will extend this with text styling (font/size/color/
// outline), video backgrounds, and multi-layer composition (see Part 4
// / reach-list items in the feature reference). Keep this struct
// plain-old-data so ScheduleModel can copy it around cheaply.
struct Slide
{
    QString label;    // e.g. "Amazing Grace - Verse 1", shown only to the operator
    QString text;     // the content actually projected
    QColor background = Qt::black;
    QString backgroundImagePath; // empty = no image, just use `background`

    Slide() = default;
    Slide(QString label_, QString text_, QColor background_ = Qt::black,
          QString backgroundImagePath_ = QString())
        : label(std::move(label_)), text(std::move(text_)), background(background_),
          backgroundImagePath(std::move(backgroundImagePath_))
    {
    }
};
