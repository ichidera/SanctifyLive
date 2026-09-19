#include "ui/HardwareSetupDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "hardware/HardwareSettings.h"

namespace
{

// The four HardwareTier values, in the fixed order every combo box /
// loop in this file iterates them -- kept in one place so the combo's
// index arithmetic (index 0 = "Auto-detected", index N+1 = tier N of
// this list) can't drift out of sync between where it's built and where
// it's read back.
const QVector<HardwareTier> &orderedTiers()
{
    static const QVector<HardwareTier> tiers = {
        HardwareTier::Minimal,
        HardwareTier::Standard,
        HardwareTier::Enhanced,
        HardwareTier::FullPro,
    };
    return tiers;
}

QString yesNo(bool recommended)
{
    return recommended ? QObject::tr("on") : QObject::tr("off");
}

} // namespace

HardwareSetupDialog::HardwareSetupDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Hardware & Performance"));
    setMinimumWidth(460);

    auto *layout = new QVBoxLayout(this);

    // --- Detected hardware -------------------------------------------------
    auto *detectedGroup = new QGroupBox(tr("Detected Hardware"), this);
    auto *detectedLayout = new QVBoxLayout(detectedGroup);

    m_detectedSummaryLabel = new QLabel(detectedGroup);
    m_detectedSummaryLabel->setWordWrap(true);
    m_detectedSummaryLabel->setTextFormat(Qt::RichText);
    detectedLayout->addWidget(m_detectedSummaryLabel);

    auto *detectedControlsRow = new QFormLayout();
    auto *redetectButton = new QPushButton(tr("Re-detect Hardware"), detectedGroup);
    detectedControlsRow->addRow(redetectButton);

    m_tierOverrideCombo = new QComboBox(detectedGroup);
    m_tierOverrideCombo->addItem(tr("Auto-detected"));
    for (HardwareTier tier : orderedTiers())
        m_tierOverrideCombo->addItem(hardwareTierName(tier));
    m_tierOverrideCombo->setToolTip(
        tr("Overrides the automatically detected tier entirely -- use this if the automatic result seems "
           "wrong for this machine (for example, over a remote desktop session)."));
    detectedControlsRow->addRow(tr("Treat hardware as:"), m_tierOverrideCombo);
    detectedLayout->addLayout(detectedControlsRow);

    layout->addWidget(detectedGroup);

    // --- Feature recommendations -------------------------------------------
    auto *featuresGroup = new QGroupBox(tr("Recommended Features"), this);
    auto *featuresLayout = new QFormLayout(featuresGroup);

    m_motionBackgroundsCheck = new QCheckBox(featuresGroup);
    featuresLayout->addRow(tr("Motion (looping video) backgrounds:"), m_motionBackgroundsCheck);

    m_videoCompositingCheck = new QCheckBox(featuresGroup);
    featuresLayout->addRow(tr("Live video compositing (IMAG):"), m_videoCompositingCheck);

    m_fadeTransitionsCheck = new QCheckBox(featuresGroup);
    featuresLayout->addRow(tr("Fade between slides:"), m_fadeTransitionsCheck);

    m_fadeDurationSpin = new QSpinBox(featuresGroup);
    m_fadeDurationSpin->setRange(0, 2000);
    m_fadeDurationSpin->setSuffix(tr(" ms"));
    m_fadeDurationSpin->setSingleStep(50);
    featuresLayout->addRow(tr("Fade duration:"), m_fadeDurationSpin);

    m_maxOutputScreensSpin = new QSpinBox(featuresGroup);
    m_maxOutputScreensSpin->setRange(1, 4);
    featuresLayout->addRow(tr("Maximum output screens:"), m_maxOutputScreensSpin);

    m_alphaKeyOutputCheck = new QCheckBox(featuresGroup);
    m_alphaKeyOutputCheck->setToolTip(
        tr("Transparent/keyed output for downstream compositing (e.g. NDI, chroma key)."));
    featuresLayout->addRow(tr("Alpha key output:"), m_alphaKeyOutputCheck);

    layout->addWidget(featuresGroup);

    m_recommendationNoteLabel = new QLabel(
        tr("These reflect what your hardware can comfortably run -- turning on more than recommended may "
           "cause stuttering during a live service. Not every feature listed here is wired up yet; this "
           "screen exists so the recommendation is visible and adjustable regardless."),
        this);
    m_recommendationNoteLabel->setWordWrap(true);
    m_recommendationNoteLabel->setObjectName("nextSlideLabel");
    layout->addWidget(m_recommendationNoteLabel);

    // --- Buttons -------------------------------------------------------------
    auto *buttonRow = new QHBoxLayout();
    auto *resetButton = new QPushButton(tr("Reset to Recommended"), this);
    buttonRow->addWidget(resetButton);
    buttonRow->addStretch(1);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttonRow->addWidget(buttonBox);
    layout->addLayout(buttonRow);

    // lastDetectedProfile() only has real data if a detection has
    // actually run this process -- on any launch after the first, the
    // tier was likely just loaded from settings with no probing at all
    // (see HardwareSettings::currentTier()). Since this dialog's whole
    // job is showing accurate live hardware info, force one fresh probe
    // whenever it opens rather than risk displaying an empty/stale
    // profile the first time someone actually looks at this screen.
    HardwareSettings::refreshDetection();

    refreshDetectedHardwareLabels();
    loadPresetIntoControls(HardwareSettings::currentPreset());
    updateFadeDurationEnabled();
    // Reflects the *current* tier's recommendation into the combo box
    // selection without forcing an override -- "Auto-detected" stays
    // selected unless the operator has actually set one previously.
    if (HardwareSettings::hasOverride()) {
        const int idx = orderedTiers().indexOf(HardwareSettings::currentTier());
        if (idx >= 0)
            m_tierOverrideCombo->setCurrentIndex(idx + 1); // +1 for the "Auto-detected" entry at index 0
    }

    connect(redetectButton, &QPushButton::clicked, this, &HardwareSetupDialog::onRedetectClicked);
    connect(m_tierOverrideCombo, &QComboBox::currentIndexChanged, this, &HardwareSetupDialog::onTierOverrideChanged);
    connect(m_fadeTransitionsCheck, &QCheckBox::toggled, this, &HardwareSetupDialog::updateFadeDurationEnabled);
    connect(resetButton, &QPushButton::clicked, this, &HardwareSetupDialog::onResetToRecommendedClicked);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &HardwareSetupDialog::onSaveClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &HardwareSetupDialog::reject);
}

void HardwareSetupDialog::refreshDetectedHardwareLabels()
{
    const HardwareProfile &profile = HardwareSettings::lastDetectedProfile();
    const HardwareTier tier = HardwareSettings::currentTier();

    const QString gpuText = profile.openglAvailable
        ? (profile.openglRenderer.isEmpty() ? tr("available, name unknown") : profile.openglRenderer)
        : tr("unavailable");
    const QString vramText = profile.vramMb >= 0 ? tr("%1 MB").arg(QLocale().toString(qlonglong(profile.vramMb)))
                                                  : tr("unknown");
    const QString ramText = profile.totalRamMb > 0 ? tr("%1 MB").arg(QLocale().toString(qlonglong(profile.totalRamMb)))
                                                    : tr("unknown");

    m_detectedSummaryLabel->setText(
        tr("<b>CPU cores:</b> %1 &nbsp;&nbsp; <b>RAM:</b> %2<br>"
           "<b>Graphics:</b> %3 &nbsp;&nbsp; <b>Video memory:</b> %4<br>"
           "<b>Result:</b> <b>%5</b> tier")
            .arg(profile.cpuCores)
            .arg(ramText, gpuText, vramText, hardwareTierName(tier)));
}

void HardwareSetupDialog::loadPresetIntoControls(const FeaturePreset &preset)
{
    m_motionBackgroundsCheck->setChecked(preset.motionBackgrounds);
    m_videoCompositingCheck->setChecked(preset.videoCompositing);
    m_fadeTransitionsCheck->setChecked(preset.fadeTransitions);
    m_fadeDurationSpin->setValue(preset.fadeDurationMs);
    m_maxOutputScreensSpin->setValue(preset.maxOutputScreens);
    m_alphaKeyOutputCheck->setChecked(preset.alphaKeyOutput);

    const FeaturePreset recommended = featurePresetForTier(HardwareSettings::currentTier());
    const QString hint = tr("Recommended for your hardware: motion backgrounds %1, video compositing %2, "
                             "fades %3 (%4 ms), up to %5 screen(s), alpha key %6.")
                              .arg(yesNo(recommended.motionBackgrounds), yesNo(recommended.videoCompositing),
                                   yesNo(recommended.fadeTransitions))
                              .arg(recommended.fadeDurationMs)
                              .arg(recommended.maxOutputScreens)
                              .arg(yesNo(recommended.alphaKeyOutput));
    m_motionBackgroundsCheck->setToolTip(hint);
}

FeaturePreset HardwareSetupDialog::presetFromControls() const
{
    FeaturePreset preset;
    preset.motionBackgrounds = m_motionBackgroundsCheck->isChecked();
    preset.videoCompositing = m_videoCompositingCheck->isChecked();
    preset.fadeTransitions = m_fadeTransitionsCheck->isChecked();
    preset.fadeDurationMs = preset.fadeTransitions ? m_fadeDurationSpin->value() : 0;
    // Thumbnail resolution isn't user-facing in this dialog yet (there's
    // no visible difference at the Media tab's current tile size to
    // justify the extra controls) -- carry forward whatever the current
    // tier recommends for it rather than exposing a control for
    // something nothing yet visibly uses.
    preset.thumbnailResolution = featurePresetForTier(HardwareSettings::currentTier()).thumbnailResolution;
    preset.maxOutputScreens = m_maxOutputScreensSpin->value();
    preset.alphaKeyOutput = m_alphaKeyOutputCheck->isChecked();
    return preset;
}

void HardwareSetupDialog::onRedetectClicked()
{
    HardwareSettings::refreshDetection();
    refreshDetectedHardwareLabels();
    // A fresh detection is exactly the kind of event that should make
    // stale feature overrides worth reconsidering, but shouldn't erase
    // them without being asked to -- only reload the controls from the
    // (possibly now-different) recommendation if there's no saved
    // override the operator would otherwise lose.
    if (!HardwareSettings::hasFeatureOverrides())
        loadPresetIntoControls(featurePresetForTier(HardwareSettings::currentTier()));
}

void HardwareSetupDialog::onTierOverrideChanged(int index)
{
    if (index <= 0) {
        HardwareSettings::clearOverride();
    } else {
        HardwareSettings::setOverrideTier(orderedTiers().at(index - 1));
    }
    refreshDetectedHardwareLabels();
    if (!HardwareSettings::hasFeatureOverrides())
        loadPresetIntoControls(featurePresetForTier(HardwareSettings::currentTier()));
}

void HardwareSetupDialog::onResetToRecommendedClicked()
{
    HardwareSettings::clearFeatureOverrides();
    loadPresetIntoControls(featurePresetForTier(HardwareSettings::currentTier()));
    updateFadeDurationEnabled();
}

void HardwareSetupDialog::onSaveClicked()
{
    HardwareSettings::setFeatureOverrides(presetFromControls());
    accept();
}

void HardwareSetupDialog::updateFadeDurationEnabled()
{
    m_fadeDurationSpin->setEnabled(m_fadeTransitionsCheck->isChecked());
}
