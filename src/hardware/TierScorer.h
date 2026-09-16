#pragma once

#include "hardware/FeaturePreset.h"

struct HardwareProfile;

// Turns a HardwareProfile (raw measurements) into a HardwareTier
// (a decision). Kept separate from HardwareProbe so the scoring
// weights/thresholds can be tuned -- based on real-world reports of a
// tier feeling wrong for a given machine -- without touching how
// hardware facts are gathered.
namespace TierScorer
{

// A 0-100-ish weighted score: roughly equal weight to CPU cores, RAM,
// and GPU capability. Exposed on its own (not just the resulting tier)
// so it can be logged for diagnostics -- "why did my machine land in
// Standard, not Enhanced" is much easier to answer with the number than
// the label alone.
int score(const HardwareProfile &profile);

// Maps a score to a tier using the thresholds: FullPro >= 70,
// Enhanced >= 50, Standard >= 30, else Minimal.
HardwareTier tierForScore(int score);

// Convenience: score() followed by tierForScore().
HardwareTier scoreAndClassify(const HardwareProfile &profile);

} // namespace TierScorer
