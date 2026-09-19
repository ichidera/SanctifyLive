#pragma once

#include "hardware/FeaturePreset.h"
#include "hardware/HardwareProfile.h"

// Runs HardwareProbe::detect() + TierScorer once per install (first
// run), then remembers the result in QSettings so every subsequent
// launch is instant and doesn't re-probe hardware or risk flickering
// between tiers if something else is briefly using the GPU. An operator
// (or, later, a Settings dialog) can pin a specific tier regardless of
// what was detected -- e.g. a machine with a strong GPU but only one
// output cable might still want FullPro's transitions without its
// 4-screen assumption confusing a future ScreenManager.
//
// This is invisible Phase 1 "hardware profile in memory" infrastructure
// -- it doesn't add or change anything in the UI. It exists so a future
// compositor, transition system, or thumbnail pipeline has one place to
// ask "what can this machine handle" instead of every call site
// re-implementing detection and QSettings plumbing.
namespace HardwareSettings
{

// Loads the persisted tier if one exists (or an override, if one is
// set); otherwise runs HardwareProbe::detect() + TierScorer, persists
// the result, and logs what was found. Safe to call more than once per
// process -- later calls just read back the now-persisted value.
HardwareTier currentTier();

// Forces a fresh HardwareProbe::detect() + TierScorer, updates the
// cached profile (see lastDetectedProfile()), and persists the new
// detected tier -- unlike currentTier(), this never reads the
// persisted/cached value first. A manual tier override (setOverrideTier
// below), if one is set, is left untouched: re-detecting hardware
// shouldn't silently discard an operator's explicit choice. Used by the
// Setup screen's "Re-detect Hardware" action; ordinary startup should
// use currentTier() instead, so a launch doesn't pay the detection cost
// every single time.
HardwareTier refreshDetection();

// The FeaturePreset that render/UI code should actually consult, most
// of the time, rather than switching on HardwareTier directly. Returns
// the Setup screen's saved feature overrides if any are set (see
// setFeatureOverrides() below), otherwise featurePresetForTier(currentTier()).
// Cheap to call on a hot path (OutputWindow calls this on every slide
// change) -- the result is cached internally and only recomputed when
// one of this namespace's other functions actually changes what it
// should return.
FeaturePreset currentPreset();

// The profile HardwareProbe::detect() found, if detection has run at
// least once this process (either via currentTier()'s first-run path or
// refreshDetection()). Default-constructed (all-zero/empty) if neither
// has happened yet.
const HardwareProfile &lastDetectedProfile();

// Pins a specific tier regardless of what was/would be detected, and
// persists the override immediately.
void setOverrideTier(HardwareTier tier);

// Removes a manual tier override, so currentTier() goes back to the
// persisted (or freshly detected) value.
void clearOverride();

bool hasOverride();

// Saves `preset` as the operator's explicit choice, taking over from
// whatever featurePresetForTier(currentTier()) would otherwise say --
// this is what the Setup screen calls when the operator reviews the
// recommended features and clicks Save, whether or not they actually
// changed anything from the recommendation. There's no per-field
// override tracking: saving takes ownership of the whole preset, rather
// than silently mixing tier defaults with one or two hand-picked
// fields, which is far simpler to reason about (both here and for
// whoever's reading the Setup screen's code) than a partial-merge
// scheme would be.
void setFeatureOverrides(const FeaturePreset &preset);

// Discards any saved override, so currentPreset() goes back to
// following the detected/pinned tier's recommendation automatically.
void clearFeatureOverrides();

bool hasFeatureOverrides();

} // namespace HardwareSettings
