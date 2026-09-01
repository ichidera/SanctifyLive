#ifndef SANCTIFYLIVE_CORE_OUTPUT_SLIDERENDERER_H_
#define SANCTIFYLIVE_CORE_OUTPUT_SLIDERENDERER_H_

#include <QRect>
#include <QSize>
#include <QString>

class QPainter;
struct Slide;
struct OutputProfile;

// SlideRenderer is the one place that knows how to turn a Slide into
// pixels: cover-fit background image (or solid color) first, then
// centered word-wrapped text inset by the destination's configured
// margins, in the destination's configured font.
//
// It exists so that "what will this actually look like on screen" only
// has one implementation in the whole app. Before this, OutputWindow had
// its own hand-rolled paintEvent(), the Media Library's preview pane was
// a plain QLabel showing an unscaled/uncropped thumbnail, Scriptures had
// no preview at all, and Settings' small preview box was a fixed mockup
// that never reflected the profile being edited. Every one of those was
// a place a preview could show something different from what actually
// gets projected. Now OutputWindow (the real congregation-facing window,
// and both its in-app mirrors) and every content-tab preview (Media,
// Scriptures, Songs, Themes) and the Settings preview thumbnail all call
// paint() below with the profile currently in effect for that
// destination -- so they can't drift apart.
//
// Deliberately namespace-level free functions rather than a class: there
// is no per-call state to hold, and every caller already owns the
// QPainter, the target rect, the Slide, and the OutputProfile it wants
// rendered.
namespace SlideRenderer {

// Paints `slide` into `targetRect` of `painter`, using `profile`'s
// margins and default font. `targetRect` is clipped to exactly, so
// nothing this call draws can ever appear outside it -- this is what
// makes cropping/overflow inside a small preview widget mean the same
// thing it means on the real screen: content that doesn't fit is simply
// not there, not "spilling over the edge of the panel."
//
// Margins are stored in OutputProfile as absolute pixels for the
// destination's actual configured resolution (profile.outputPosition).
// When `targetRect` is smaller than that (as it always is for an in-app
// preview), margins -- and the font's `height/8` sizing -- are scaled
// down by the same ratio, so a preview at, say, 1/8th scale reproduces
// the exact same *proportions* the real screen would show, not
// disproportionately huge margins eating the whole frame.
void paint(QPainter &painter, const QRect &targetRect, const Slide &slide, const OutputProfile &profile);

// Given the space a preview widget actually has available and the
// native resolution it needs to represent, returns the largest rect of
// that resolution's exact aspect ratio that fits inside `available`,
// centered within it (classic letterbox/pillarbox fit -- same idea as
// CSS `object-fit: contain`, never `cover`). Used by every preview
// widget so the box it hands to paint() always has the destination's
// real aspect ratio, regardless of the preview widget's own shape.
QRect fittedFrame(const QRect &available, const QSize &nativeResolution);

// Draws the small "WxH" resolution badge used in every preview so the
// operator can always see, at a glance, what output a given preview
// represents. Pure preview-tool chrome -- not part of what actually gets
// projected, so this is intentionally separate from paint() above.
void drawResolutionBadge(QPainter &painter, const QRect &frame, const QString &label);

} // namespace SlideRenderer

#endif // SANCTIFYLIVE_CORE_OUTPUT_SLIDERENDERER_H_
