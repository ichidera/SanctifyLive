#pragma once

#include "hardware/HardwareProfile.h"

// HardwareProbe measures the machine SanctifyLive is actually running
// on -- CPU, RAM, and GPU -- so TierScorer (see TierScorer.h) can decide
// which features are safe to turn on. This matters because church
// presentation software runs on whatever hardware a volunteer donated
// or the building already owned, from a decade-old laptop to a
// dedicated media PC, far more often than it runs on hardware chosen
// for the job.
//
// Pure detection, no judgment calls: detect() never decides "is this
// machine good enough for X" -- that's TierScorer's job, kept separate
// so the scoring weights can be tuned without touching how facts are
// gathered.
namespace HardwareProbe
{

// Synchronous and can take a few milliseconds (it creates a throwaway
// offscreen OpenGL context to query the GPU) -- call it once at
// startup, not on a hot path. See hardware/HardwareSettings.h for the
// "call once, then remember the result" wrapper the rest of the app
// should actually use.
HardwareProfile detect();

} // namespace HardwareProbe
