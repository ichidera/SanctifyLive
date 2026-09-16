#include "hardware/FeaturePreset.h"

QString hardwareTierName(HardwareTier tier)
{
    switch (tier) {
    case HardwareTier::Minimal:
        return QStringLiteral("Minimal");
    case HardwareTier::Standard:
        return QStringLiteral("Standard");
    case HardwareTier::Enhanced:
        return QStringLiteral("Enhanced");
    case HardwareTier::FullPro:
        return QStringLiteral("FullPro");
    }
    return QStringLiteral("Minimal"); // unreachable for a valid enum value; degrade safely rather than UB
}

FeaturePreset featurePresetForTier(HardwareTier tier)
{
    FeaturePreset preset;
    switch (tier) {
    case HardwareTier::Minimal:
        preset.motionBackgrounds = false;
        preset.videoCompositing = false;
        preset.fadeTransitions = false;
        preset.fadeDurationMs = 0;
        preset.thumbnailResolution = QSize(); // thumbnails off entirely
        preset.maxOutputScreens = 1;
        preset.alphaKeyOutput = false;
        break;

    case HardwareTier::Standard:
        preset.motionBackgrounds = false;
        preset.videoCompositing = false;
        preset.fadeTransitions = true;
        preset.fadeDurationMs = 200;
        preset.thumbnailResolution = QSize(160, 90); // low-res thumbnails
        preset.maxOutputScreens = 1;
        preset.alphaKeyOutput = false;
        break;

    case HardwareTier::Enhanced:
        preset.motionBackgrounds = true;
        preset.videoCompositing = false;
        preset.fadeTransitions = true;
        preset.fadeDurationMs = 400;
        preset.thumbnailResolution = QSize(320, 180); // full-res thumbnails
        preset.maxOutputScreens = 2;
        preset.alphaKeyOutput = false;
        break;

    case HardwareTier::FullPro:
        preset.motionBackgrounds = true;
        preset.videoCompositing = true;
        preset.fadeTransitions = true;
        preset.fadeDurationMs = 400;
        preset.thumbnailResolution = QSize(320, 180);
        preset.maxOutputScreens = 4;
        preset.alphaKeyOutput = true;
        break;
    }
    return preset;
}
