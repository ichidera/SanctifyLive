#ifndef SANCTIFYLIVE_CORE_SLIDE_H_
#define SANCTIFYLIVE_CORE_SLIDE_H_


#include <QColor>
#include <QPointF>
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

    // Normalized (0..1 across width/height) point in backgroundImagePath
    // that should stay centered whenever a cover-fit crop has to discard
    // part of the image because the output's aspect ratio doesn't match
    // the source's -- e.g. a portrait phone showing a 16:9 photo. Default
    // (0.5, 0.5) reproduces the old always-center-crop behavior exactly.
    // Set via "Edit Framing..." in the Media Library; meaningless when
    // backgroundImagePath is empty.
    QPointF backgroundFocus = QPointF(0.5, 0.5);

    Slide() = default;
    Slide(QString label_, QString text_, QColor background_ = Qt::black)
        : label(std::move(label_)), text(std::move(text_)), background(background_)
    {
    }

    // Shared by every place that turns a media-library-style entry (a
    // label, a fallback color, an optional real image, and a crop focus
    // point) into a Slide -- currently OperatorWindow::onMediaActivated /
    // onMediaSentToPhone for the real Media tab, and the Themes tab,
    // plus the live-appearance preview both of those feed. Keeping this
    // one function is what guarantees a theme/media preview can never
    // silently drift from what actually goes out when the same entry is
    // sent live: a real image supplies its own visual (no text overlay);
    // a placeholder swatch shows its label as the overlay text instead,
    // since it has no picture of its own to display.
    static Slide fromMediaEntry(const QString &label, const QColor &background, const QString &imagePath,
                                 const QPointF &focus)
    {
        Slide slide(label, imagePath.isEmpty() ? label : QString(), background);
        slide.backgroundImagePath = imagePath;
        slide.backgroundFocus = focus;
        return slide;
    }
};

#endif // SANCTIFYLIVE_CORE_SLIDE_H_