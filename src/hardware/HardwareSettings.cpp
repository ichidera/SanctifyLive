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

// Actually probes the machine, logs what it found, persists the result,
// and returns the detected tier -- the one piece of work shared by
// currentTier()'s first-run path and refreshDetection().
HardwareTier detectLogAndPersist(QSettings &settings)
{
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

// Feature-override persistence keys. All six are written together by
// setFeatureOverrides() and only ever read back as a complete set (see
// that function's doc comment for why there's no partial/per-field
// tracking).
constexpr const char *kHasFeatureOverridesKey = "Hardware/Override/HasOverrides";
constexpr const char *kMotionBackgroundsKey = "Hardware/Override/MotionBackgrounds";
constexpr const char *kVideoCompositingKey = "Hardware/Override/VideoCompositing";
constexpr const char *kFadeTransitionsKey = "Hardware/Override/FadeTransitions";
constexpr const char *kFadeDurationMsKey = "Hardware/Override/FadeDurationMs";
constexpr const char *kThumbnailWidthKey = "Hardware/Override/ThumbnailWidth";
constexpr const char *kThumbnailHeightKey = "Hardware/Override/ThumbnailHeight";
constexpr const char *kMaxOutputScreensKey = "Hardware/Override/MaxOutputScreens";
constexpr const char *kAlphaKeyOutputKey = "Hardware/Override/AlphaKeyOutput";

// currentPreset() is on a genuine hot path now: OutputWindow calls it on
// every single slide change (see the fade-transition logic there), which
// can happen many times a minute during a live service. Re-parsing
// QSettings on every one of those calls is real, avoidable work for a
// value that essentially never changes between explicit user actions
// (opening HardwareSetupDialog and clicking Save/Reset, or an explicit
// refreshDetection()) -- so it's cached here, and every function that
// can actually change what currentPreset() should return
// (setOverrideTier, clearOverride, setFeatureOverrides,
// clearFeatureOverrides, refreshDetection) explicitly invalidates it.
// There is no time-based expiry: correctness relies entirely on every
// mutator remembering to invalidate, which is why the mutators all live
// in this one file, next to the cache, rather than being spread out.
struct PresetCache
{
    bool valid = false;
    FeaturePreset preset;
};
PresetCache &presetCache()
{
    static PresetCache cache;
    return cache;
}
void invalidatePresetCache()
{
    presetCache().valid = false;
}

} // namespace

HardwareTier currentTier()
{
    QSettings settings;

    HardwareTier overrideTier;
    if (settings.value(kHasOverrideKey, false).toBool()
        && tierFromSettingsValue(settings.value(kOverrideTierKey).toString(), &overrideTier)) {
        return overrideTier;
    }

    HardwareTier persistedTier;
    if (settings.contains(kDetectedTierKey)
        && tierFromSettingsValue(settings.value(kDetectedTierKey).toString(), &persistedTier)) {
        return persistedTier;
    }

    // First run, or the settings file was deleted/doesn't have this key
    // yet: actually probe the machine. This is the only path that logs
    // -- the two returns above happen on essentially every call once a
    // tier is known (e.g. once per rendered frame's currentPreset()
    // lookup), so logging there would spam the console for a value that
    // hasn't changed; a real detection pass, in contrast, is rare (once
    // per install, or an explicit refreshDetection()) and worth a
    // permanent record of what was found.
    return detectLogAndPersist(settings);
}

HardwareTier refreshDetection()
{
    QSettings settings;
    const HardwareTier tier = detectLogAndPersist(settings);
    invalidatePresetCache();
    return tier;
}

FeaturePreset currentPreset()
{
    PresetCache &cache = presetCache();
    if (cache.valid)
        return cache.preset;

    FeaturePreset preset;
    if (hasFeatureOverrides()) {
        QSettings settings;
        preset.motionBackgrounds = settings.value(kMotionBackgroundsKey, false).toBool();
        preset.videoCompositing = settings.value(kVideoCompositingKey, false).toBool();
        preset.fadeTransitions = settings.value(kFadeTransitionsKey, false).toBool();
        preset.fadeDurationMs = settings.value(kFadeDurationMsKey, 0).toInt();
        const int thumbW = settings.value(kThumbnailWidthKey, 0).toInt();
        const int thumbH = settings.value(kThumbnailHeightKey, 0).toInt();
        preset.thumbnailResolution = (thumbW > 0 && thumbH > 0) ? QSize(thumbW, thumbH) : QSize();
        preset.maxOutputScreens = qMax(1, settings.value(kMaxOutputScreensKey, 1).toInt());
        preset.alphaKeyOutput = settings.value(kAlphaKeyOutputKey, false).toBool();
    } else {
        preset = featurePresetForTier(currentTier());
    }

    cache.preset = preset;
    cache.valid = true;
    return preset;
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
    invalidatePresetCache();
}

void clearOverride()
{
    QSettings settings;
    settings.setValue(kHasOverrideKey, false);
    settings.remove(kOverrideTierKey);
    invalidatePresetCache();
}

bool hasOverride()
{
    QSettings settings;
    return settings.value(kHasOverrideKey, false).toBool();
}

void setFeatureOverrides(const FeaturePreset &preset)
{
    QSettings settings;
    settings.setValue(kHasFeatureOverridesKey, true);
    settings.setValue(kMotionBackgroundsKey, preset.motionBackgrounds);
    settings.setValue(kVideoCompositingKey, preset.videoCompositing);
    settings.setValue(kFadeTransitionsKey, preset.fadeTransitions);
    settings.setValue(kFadeDurationMsKey, preset.fadeDurationMs);
    settings.setValue(kThumbnailWidthKey, preset.thumbnailResolution.width());
    settings.setValue(kThumbnailHeightKey, preset.thumbnailResolution.height());
    settings.setValue(kMaxOutputScreensKey, preset.maxOutputScreens);
    settings.setValue(kAlphaKeyOutputKey, preset.alphaKeyOutput);
    invalidatePresetCache();
}

void clearFeatureOverrides()
{
    QSettings settings;
    settings.setValue(kHasFeatureOverridesKey, false);
    invalidatePresetCache();
}

bool hasFeatureOverrides()
{
    QSettings settings;
    return settings.value(kHasFeatureOverridesKey, false).toBool();
}

} // namespace HardwareSettings
