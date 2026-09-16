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

// The FeaturePreset for currentTier() -- what render/UI code should
// actually consult, most of the time, rather than switching on the
// tier enum directly.
FeaturePreset currentPreset();

// The profile HardwareProbe::detect() found, if detection actually ran
// this process (i.e. no tier was already persisted and no override is
// set). Default-constructed (all-zero/empty) if currentTier() was
// satisfied entirely from settings -- check openglAvailable/cpuCores
// before trusting this for diagnostics, or just call
// HardwareProbe::detect() directly if you need a fresh reading
// regardless of what's cached.
const HardwareProfile &lastDetectedProfile();

// Pins a specific tier regardless of what was/would be detected, and
// persists the override immediately.
void setOverrideTier(HardwareTier tier);

// Removes a manual override, so currentTier() goes back to the
// persisted (or freshly detected) value.
void clearOverride();

bool hasOverride();

} // namespace HardwareSettings
