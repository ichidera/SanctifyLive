#pragma once

#include <QSize>
#include <QString>

// Which capability tier a machine falls into, and the concrete feature
// set (FeaturePreset, below) SanctifyLive should offer at that tier.
//
// Why this exists: church presentation software runs on whatever
// hardware a volunteer donated or the building already owned -- a
// decade-old laptop as often as a dedicated media PC -- far more often
// than it runs on hardware chosen for the job. A motion background loop
// or live-video compositing that's smooth on one machine can be a
// stuttering mess (or a crash) on another. Rather than either assuming
// good hardware everywhere or disabling those features for everyone,
// SanctifyLive measures the machine once (see HardwareProbe +
// TierScorer) and only offers what it can actually run well.
enum class HardwareTier
{
    Minimal,  // static color backgrounds only, instant cuts, 1 screen
    Standard, // static image backgrounds, fast fade, 1 screen
    Enhanced, // motion background loops, fade, 2 screens
    FullPro,  // motion + live video compositing, all transitions, up to 4 screens, alpha key
};

// "Minimal"/"Standard"/"Enhanced"/"FullPro" -- used both for log
// messages and as the literal value stored in QSettings (see
// HardwareSettings.cpp), so this is intentionally a stable, persisted
// string, not just a debug label.
QString hardwareTierName(HardwareTier tier);

// What a tier actually turns on/off, as plain booleans/ints -- a lookup
// table to be consulted at render/UI time, not logic of its own. Kept
// deliberately dumb: if a future milestone needs a feature this doesn't
// cover yet, add a field here and a case in featurePresetForTier()
// rather than teaching call sites to reason about HardwareTier directly.
struct FeaturePreset
{
    bool motionBackgrounds = false; // looping video behind text (vs. static image/color only)
    bool videoCompositing = false;  // live/IMAG video layer support
    bool fadeTransitions = false;   // crossfade between slides (vs. instant cut)
    int fadeDurationMs = 0;         // 0 when fadeTransitions is false
    QSize thumbnailResolution;      // media/preview thumbnail render size; null QSize = thumbnails off
    int maxOutputScreens = 1;
    bool alphaKeyOutput = false; // transparent/keyed output for downstream compositing (e.g. NDI, chroma)
};

// The concrete numbers behind each tier -- see the .cpp for the actual
// table. Pure function of the enum value, so it's cheap to call
// wherever a preset is needed rather than caching it yourself.
FeaturePreset featurePresetForTier(HardwareTier tier);
