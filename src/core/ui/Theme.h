#pragma once

class QApplication;

// Theme is the ONE place that defines SanctifyLive's dark, operator-console
// look (the palette + stylesheet applied across OperatorWindow and its
// panels). Centralizing it here means every widget gets a consistent look
// without each one hand-rolling its own colors, and a future "light theme"
// or "high-contrast theme" option is one new function here, not a hunt
// through every .cpp file for hardcoded hex colors.
namespace Theme
{
// Applies the dark palette + global stylesheet to the whole application.
// Call once, early in main().
void applyDark(QApplication &app);

// Shared color constants, exposed so panels that need a specific accent
// (e.g. the "Live" label in green, a staged-row highlight) don't have to
// invent their own hex strings that can drift from the stylesheet.
namespace Colors
{
constexpr const char *background = "#1e1f24";
constexpr const char *panel = "#26272e";
constexpr const char *panelAlt = "#2c2d35";
constexpr const char *border = "#3a3b44";
constexpr const char *textPrimary = "#e8e8ec";
constexpr const char *textMuted = "#8a8b93";
constexpr const char *accentLive = "#22c55e";  // green -- "Live" label/state
constexpr const char *accentRecord = "#e03e3e"; // red -- Go Live / Black / recording-style accents
constexpr const char *accentSelect = "#3b6fd4"; // blue -- staged/selected row
}
}
