#pragma once

#include <QSize>

// The single logical canvas every slide is rendered into, regardless of
// what physical screen/projector/preview thumbnail it ends up on.
//
// This is the crux of resolution independence: nothing in the rendering
// code should ever ask "how big is my window" and use that to decide
// font size, margins, or layout. It should render into this fixed
// coordinate space, and every consumer (OutputWindow on a projector,
// OutputWindow embedded as a live preview, a future stage-view window)
// scales the resulting QPixmap to its own physical rect(). Same bytes,
// different rect() -- see SlideRenderer / OutputWindow.
//
// 1920x1080 is a reasonable default "canvas" size: high enough
// resolution that scaling down (the common case -- most projectors are
// 1080p or lower) never looks soft, while staying cheap to rasterize
// every time a slide changes. If a later milestone wants per-service or
// per-ServiceItem design resolutions (e.g. matching a venue's native
// LED wall resolution), this becomes a parameter passed alongside the
// Slide instead of a global constant -- nothing that consumes it needs
// to change shape.
constexpr QSize kDesignResolution(1920, 1080);
