#pragma once

#include <QString>

// Raw hardware facts gathered by HardwareProbe::detect() -- see
// HardwareProbe.h. Kept separate from HardwareTier/FeaturePreset (see
// FeaturePreset.h) so "what we measured" and "what we decided to do
// about it" stay independent: the scoring weights in TierScorer can
// change without touching detection, and vice versa.
//
// No Qt widgets, no rendering -- pure data, same rule as core/model/
// and core/scripture/, even though this lives outside core/ (see
// CMakeLists.txt's directory-layout comment for why hardware/ is its
// own top-level sibling rather than nested under core/).
struct HardwareProfile
{
    int cpuCores = 1;    // QThread::idealThreadCount()
    qint64 totalRamMb = 0; // physical RAM in MB; 0 = couldn't be determined on this platform

    QString openglRenderer; // GL_RENDERER string, e.g. "llvmpipe" or "NVIDIA GeForce RTX 3060/PCIe/SSE2"
    QString openglVersion;  // GL_VERSION string
    bool openglAvailable = false; // false if not even an offscreen GL context could be created

    qint64 vramMb = -1; // -1 = unknown (no NVX extension exposed by the driver, or no GPU at all)
};
