#pragma once

#include <QString>

// Centralizes SanctifyLive's dark, broadcast-console visual style so
// every widget looks like part of the same application instead of each
// dialog/panel inventing its own colors. This is UI-only: nothing here
// is referenced by core/model or core/render.
namespace Theme
{
// Key colors, exposed individually for the handful of places (e.g. the
// "LIVE" indicator, per-slide background swatches) that need a QColor
// rather than a stylesheet rule.
constexpr const char *kBackground = "#15171c";
constexpr const char *kPanel = "#1b1e26";
constexpr const char *kPanelAlt = "#20232c";
constexpr const char *kBorder = "#2a2e38";
constexpr const char *kTextPrimary = "#e8e9ec";
constexpr const char *kTextMuted = "#8b8f9a";
constexpr const char *kAccentBlue = "#3b82f6";
constexpr const char *kAccentRed = "#e0432b";
constexpr const char *kAccentGreen = "#2fbf6a";

// The full application stylesheet (QSS). Applied once, at startup, to
// the QApplication instance in main.cpp.
QString stylesheet();

} // namespace Theme
