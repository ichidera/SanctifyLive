#pragma once

#include <QDialog>

#include "hardware/FeaturePreset.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;

// HardwareSetupDialog is the "place where we tell users recommended
// features they should enable or not" for the hardware-tier system in
// src/hardware/ (HardwareProbe/TierScorer/FeaturePreset/HardwareSettings
// -- see those headers for the detection and scoring behind this
// screen). Until this dialog existed, that system ran invisibly at
// startup (detect once, log, persist) with nothing in the UI to show
// for it; this is where an operator actually sees what was detected and
// decides what to do about it.
//
// Two things happen here, deliberately kept visually and conceptually
// separate:
//
//   1. DETECTED HARDWARE is read-only (CPU cores, RAM, GPU, VRAM, the
//      resulting score and tier) -- a "Re-detect Hardware" button forces
//      a fresh HardwareProbe::detect() pass (HardwareSettings::
//      refreshDetection()) rather than trusting a stale first-run
//      result forever, e.g. after a graphics driver update or a GPU
//      swap. A "Treat hardware as" dropdown lets an operator override
//      the computed tier wholesale (HardwareSettings::setOverrideTier())
//      -- useful when the heuristic gets it wrong for a specific
//      machine (say, a strong GPU behind a remote-desktop session that
//      makes OpenGL look software-rendered).
//
//   2. FEATURE RECOMMENDATIONS are editable: each checkbox/spinbox
//      starts at what the current tier recommends (featurePresetForTier()),
//      labeled as a recommendation rather than a fixed rule, and the
//      operator can turn any of them on or off. Saving persists the
//      whole edited preset via HardwareSettings::setFeatureOverrides()
//      -- seeing the review-and-decide step through to a save always
//      takes ownership of all of it, rather than silently layering one
//      hand-changed field over otherwise-automatic tier defaults (see
//      that function's doc comment for the reasoning). "Reset to
//      Recommended" discards any saved override and reloads the tier's
//      defaults into the same controls, without closing the dialog, so
//      the effect of doing so is visible immediately.
//
// This dialog only edits settings (via HardwareSettings) -- it does not
// itself gate any feature. Code that actually turns a feature on or off
// (e.g. SlideCanvas's crossfade, gated by fadeTransitions) reads
// HardwareSettings::currentPreset() independently, same as it would
// whether or not this dialog had ever been opened.
class HardwareSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HardwareSetupDialog(QWidget *parent = nullptr);

private:
    void refreshDetectedHardwareLabels();
    void loadPresetIntoControls(const FeaturePreset &preset);
    FeaturePreset presetFromControls() const;
    void onRedetectClicked();
    void onTierOverrideChanged(int index);
    void onResetToRecommendedClicked();
    void onSaveClicked();
    void updateFadeDurationEnabled();

    QLabel *m_detectedSummaryLabel = nullptr;
    QComboBox *m_tierOverrideCombo = nullptr;

    QCheckBox *m_motionBackgroundsCheck = nullptr;
    QCheckBox *m_videoCompositingCheck = nullptr;
    QCheckBox *m_fadeTransitionsCheck = nullptr;
    QSpinBox *m_fadeDurationSpin = nullptr;
    QSpinBox *m_maxOutputScreensSpin = nullptr;
    QCheckBox *m_alphaKeyOutputCheck = nullptr;

    QLabel *m_recommendationNoteLabel = nullptr;
};
