#include "hardware/HardwareSettings.h"

#include <QDebug>
#include <QSettings>
#include <QVector>

#include "hardware/HardwareProbe.h"
#include "hardware/TierScorer.h"

namespace HardwareSettings
{

namespace
{

constexpr const char *kDetectedTierKey = "Hardware/DetectedTier";
constexpr const char *kOverrideTierKey = "Hardware/OverrideTier";
constexpr const char *kHasOverrideKey = "Hardware/HasOverride";

HardwareProfile &cachedProfile()
{
    // Default-constructed until/unless currentTier() actually runs
    // HardwareProbe::detect() this process -- see lastDetectedProfile()'s
    // doc comment.
    static HardwareProfile profile;
    return profile;
}

bool tierFromSettingsValue(const QString &value, HardwareTier *outTier)
{
    static const QVector<HardwareTier> kAllTiers = {
        HardwareTier::Minimal,
        HardwareTier::Standard,
        HardwareTier::Enhanced,
        HardwareTier::FullPro,
    };
    for (HardwareTier tier : kAllTiers) {
        if (hardwareTierName(tier) == value) {
            *outTier = tier;
            return true;
        }
    }
    return false;
}

} // namespace

HardwareTier currentTier()
{
    QSettings settings;

    HardwareTier overrideTier;
    if (settings.value(kHasOverrideKey, false).toBool()
        && tierFromSettingsValue(settings.value(kOverrideTierKey).toString(), &overrideTier)) {
        qInfo().noquote() << QStringLiteral("SanctifyLive hardware tier: %1 (manual override)")
                                  .arg(hardwareTierName(overrideTier));
        return overrideTier;
    }

    HardwareTier persistedTier;
    if (settings.contains(kDetectedTierKey)
        && tierFromSettingsValue(settings.value(kDetectedTierKey).toString(), &persistedTier)) {
        qInfo().noquote() << QStringLiteral("SanctifyLive hardware tier: %1 (from saved settings)")
                                  .arg(hardwareTierName(persistedTier));
        return persistedTier;
    }

    // First run, or the settings file was deleted/doesn't have this key
    // yet: actually probe the machine.
    cachedProfile() = HardwareProbe::detect();
    const int scoreValue = TierScorer::score(cachedProfile());
    const HardwareTier detectedTier = TierScorer::tierForScore(scoreValue);
    settings.setValue(kDetectedTierKey, hardwareTierName(detectedTier));

    const HardwareProfile &profile = cachedProfile();
    qInfo().noquote() << QStringLiteral(
        "SanctifyLive hardware detection: %1 core(s), %2 MB RAM, GL renderer \"%3\" (%4), VRAM %5 "
        "-> score %6 -> tier %7 (saved)")
                              .arg(profile.cpuCores)
                              .arg(profile.totalRamMb)
                              .arg(profile.openglRenderer.isEmpty() ? QStringLiteral("unavailable")
                                                                     : profile.openglRenderer)
                              .arg(profile.openglVersion)
                              .arg(profile.vramMb >= 0 ? QStringLiteral("%1 MB").arg(profile.vramMb)
                                                         : QStringLiteral("unknown"))
                              .arg(scoreValue)
                              .arg(hardwareTierName(detectedTier));

    return detectedTier;
}

FeaturePreset currentPreset()
{
    return featurePresetForTier(currentTier());
}

const HardwareProfile &lastDetectedProfile()
{
    return cachedProfile();
}

void setOverrideTier(HardwareTier tier)
{
    QSettings settings;
    settings.setValue(kHasOverrideKey, true);
    settings.setValue(kOverrideTierKey, hardwareTierName(tier));
}

void clearOverride()
{
    QSettings settings;
    settings.setValue(kHasOverrideKey, false);
    settings.remove(kOverrideTierKey);
}

bool hasOverride()
{
    QSettings settings;
    return settings.value(kHasOverrideKey, false).toBool();
}

} // namespace HardwareSettings
