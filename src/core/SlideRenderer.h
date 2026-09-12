#pragma once

#include <QPixmap>
#include <QSize>

struct Slide;

// SlideRenderer turns a Slide into a fully-composed QPixmap at a given
// logical resolution. This is the ONLY place that knows how to draw a
// Slide's content -- OutputWindow never draws text/backgrounds itself,
// it just displays whatever pixmap this produces.
//
// Deliberately a stateless free function rather than a class: for this
// milestone a Slide is plain data and rendering it has no state to
// carry between calls. If later milestones introduce multi-layer
// composition (background media + text + lower-third, per the feature
// reference) this can grow into a class that caches decoded media, but
// the call shape from OutputWindow's point of view shouldn't need to
// change: give it a Slide and a resolution, get back a pixmap.
//
// Passing "nullptr" for slide renders a plain black frame -- this is
// the shared implementation of blackout / empty-schedule, so
// OutputWindow doesn't need its own special-cased black-fill branch.
namespace SlideRenderer
{
QPixmap render(const Slide *slide, const QSize &resolution);
}
