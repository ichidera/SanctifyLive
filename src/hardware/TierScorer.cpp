#include "hardware/TierScorer.h"

#include <QStringList>
#include <QtGlobal>

#include "hardware/HardwareProfile.h"

namespace TierScorer
{

namespace
{

// Common software rasterizer names -- these show up on remote desktops,
// some virtual machines, and headless CI, none of which will handle a
// motion background loop smoothly no matter how many CPU cores or how
// much RAM the machine has. Matched as a substring, case-insensitively,
// against GL_RENDERER, since exact strings vary by driver version
// ("llvmpipe (LLVM 15.0.7, 256 bits)" etc).
bool looksLikeSoftwareRenderer(const QString &rendererName)
{
    static const QStringList softwareMarkers = {
        QStringLiteral("llvmpipe"),
        QStringLiteral("softpipe"),
        QStringLiteral("swrast"),
        QStringLiteral("microsoft basic render driver"),
        QStringLiteral("mesa gallium"),
    };
    const QString lower = rendererName.toLower();
    for (const QString &marker : softwareMarkers) {
        if (lower.contains(marker))
            return true;
    }
    return false;
}

} // namespace

int score(const HardwareProfile &profile)
{
    // Three roughly-equal-weight components (CPU, RAM, GPU) summing to
    // a 0-100ish scale that lines up with the tier thresholds below.
    // These weights aren't derived from a benchmark suite -- they're a
    // deliberately simple, explainable heuristic (more cores/RAM/a real
    // GPU is better), which is all this actually needs to decide:
    // "will this machine visibly struggle with a motion background",
    // not a precise performance ranking.
    double total = 0.0;

    // CPU: up to 30 points, saturating at 8 cores -- past that, more
    // cores stop meaningfully helping a mostly single-threaded
    // compositor.
    total += 30.0 * qMin(1.0, profile.cpuCores / 8.0);

    // RAM: up to 30 points, saturating at 16 GB.
    const double ramGb = profile.totalRamMb / 1024.0;
    total += 30.0 * qMin(1.0, ramGb / 16.0);

    // GPU: up to 40 points. No usable OpenGL context at all, or an
    // obviously-software rasterizer, scores 0 here regardless of
    // CPU/RAM -- those machines should stay on static backgrounds no
    // matter how many cores they have. A real GPU gets a base
    // allowance plus a bonus if VRAM could be determined and is
    // generous; unknown VRAM on a confirmed-real GPU still gets a
    // modest allowance rather than being punished for a driver that
    // doesn't expose the NVX extension.
    if (profile.openglAvailable && !looksLikeSoftwareRenderer(profile.openglRenderer)) {
        total += 28.0;
        if (profile.vramMb >= 0)
            total += 12.0 * qMin(1.0, profile.vramMb / 4096.0); // saturates at 4 GB VRAM
        else
            total += 6.0;
    }

    return int(qRound(total));
}

HardwareTier tierForScore(int scoreValue)
{
    if (scoreValue >= 70)
        return HardwareTier::FullPro;
    if (scoreValue >= 50)
        return HardwareTier::Enhanced;
    if (scoreValue >= 30)
        return HardwareTier::Standard;
    return HardwareTier::Minimal;
}

HardwareTier scoreAndClassify(const HardwareProfile &profile)
{
    return tierForScore(score(profile));
}

} // namespace TierScorer
