#pragma once

#include <QColor>
#include <QString>

// A Slide is the smallest unit of content that can be put on the live
// output. For this first milestone it's deliberately minimal: a label
// (shown in the operator's schedule list, never on screen), the text
// body, and a background color.
//
// Later milestones will extend this with a background image/video path,
// text styling (font/size/color/outline), and multi-layer composition
// (see Part 4 / reach-list items in the feature reference). Keep this
// struct plain-old-data so ScheduleModel can copy it around cheaply.
struct Slide
{
    QString label;    // e.g. "Amazing Grace - Verse 1", shown only to the operator
    QString text;     // the content actually projected as an overlay
    QColor background = Qt::black;

    // If non-empty, this real image file is drawn as the background
    // (scaled to fill, like CSS background-size: cover) instead of the
    // solid `background` color. Left empty for text slides and the
    // placeholder color swatches used before real media import existed.
    QString backgroundImagePath;

    Slide() = default;
    Slide(QString label_, QString text_, QColor background_ = Qt::black)
        : label(std::move(label_)), text(std::move(text_)), background(background_)
    {
    }
};