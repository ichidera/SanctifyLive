#include "OutputContentTabs.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSpinBox>
#include <QVBoxLayout>

// ---------------------------------------------------------------------------
// OutputGeneralTab
// ---------------------------------------------------------------------------

OutputGeneralTab::OutputGeneralTab(OutputProfile *profile, QWidget *parent)
    : QWidget(parent), m_profile(profile)
{
    // --- Select Output Monitor ---
    auto *monitorGroup = new QGroupBox(tr("Select Output Monitor"), this);
    m_monitorCombo = new QComboBox(monitorGroup);
    m_alphaCombo = new QComboBox(monitorGroup);
    m_alphaCombo->addItem(OutputProfile::alphaChannelLabel(OutputProfile::AlphaChannel::Disabled));
    m_alphaCombo->addItem(OutputProfile::alphaChannelLabel(OutputProfile::AlphaChannel::Straight));
    m_alphaCombo->addItem(OutputProfile::alphaChannelLabel(OutputProfile::AlphaChannel::Premultiplied));
    m_alphaCombo->setCurrentIndex(static_cast<int>(m_profile->alphaChannel));
    populateMonitorCombo();

    auto *posGroup = new QGroupBox(tr("Output Position"), monitorGroup);
    m_posLeft = new QSpinBox(posGroup);
    m_posTop = new QSpinBox(posGroup);
    m_posWidth = new QSpinBox(posGroup);
    m_posHeight = new QSpinBox(posGroup);
    for (QSpinBox *box : {m_posLeft, m_posTop, m_posWidth, m_posHeight})
        box->setRange(-10000, 10000);
    m_posLeft->setValue(m_profile->outputPosition.left());
    m_posTop->setValue(m_profile->outputPosition.top());
    m_posWidth->setValue(m_profile->outputPosition.width());
    m_posHeight->setValue(m_profile->outputPosition.height());

    auto *posLayout = new QGridLayout(posGroup);
    posLayout->addWidget(new QLabel(tr("Left")), 0, 0);
    posLayout->addWidget(m_posLeft, 0, 1);
    posLayout->addWidget(new QLabel(tr("Top")), 0, 2);
    posLayout->addWidget(m_posTop, 0, 3);
    posLayout->addWidget(new QLabel(tr("Width")), 1, 0);
    posLayout->addWidget(m_posWidth, 1, 1);
    posLayout->addWidget(new QLabel(tr("Height")), 1, 2);
    posLayout->addWidget(m_posHeight, 1, 3);

    m_monitorSetupButton = new QPushButton(tr("Monitor Setup"), monitorGroup);
    m_monitorSetupButton->setToolTip(
        tr("Shows how many displays this computer currently sees, for wiring the right one "
           "up here -- opens the OS display settings on most platforms."));

    auto *monitorLayout = new QGridLayout(monitorGroup);
    monitorLayout->addWidget(new QLabel(tr("Output Monitor")), 0, 0);
    monitorLayout->addWidget(m_monitorCombo, 0, 1);
    monitorLayout->addWidget(new QLabel(tr("Alpha Channel")), 1, 0);
    monitorLayout->addWidget(m_alphaCombo, 1, 1);
    monitorLayout->addWidget(posGroup, 0, 2, 3, 1);
    monitorLayout->addWidget(m_monitorSetupButton, 2, 0, 1, 2);
    monitorLayout->setColumnStretch(1, 1);
    monitorLayout->setColumnStretch(2, 1);

    // --- Screen Margins ---
    auto *marginsGroup = new QGroupBox(tr("Screen Margins"), this);
    m_marginLeft = new QSpinBox(marginsGroup);
    m_marginTop = new QSpinBox(marginsGroup);
    m_marginRight = new QSpinBox(marginsGroup);
    m_marginBottom = new QSpinBox(marginsGroup);
    for (QSpinBox *box : {m_marginLeft, m_marginTop, m_marginRight, m_marginBottom})
        box->setRange(0, 2000);
    m_marginLeft->setValue(m_profile->marginLeft);
    m_marginTop->setValue(m_profile->marginTop);
    m_marginRight->setValue(m_profile->marginRight);
    m_marginBottom->setValue(m_profile->marginBottom);

    auto *marginsLayout = new QGridLayout(marginsGroup);
    marginsLayout->addWidget(new QLabel(tr("Left")), 0, 0);
    marginsLayout->addWidget(m_marginLeft, 0, 1);
    marginsLayout->addWidget(new QLabel(tr("Top")), 0, 2);
    marginsLayout->addWidget(m_marginTop, 0, 3);
    marginsLayout->addWidget(new QLabel(tr("Right")), 1, 0);
    marginsLayout->addWidget(m_marginRight, 1, 1);
    marginsLayout->addWidget(new QLabel(tr("Bottom")), 1, 2);
    marginsLayout->addWidget(m_marginBottom, 1, 3);

    // --- Options ---
    auto *optionsGroup = new QGroupBox(tr("Options"), this);
    m_fontCombo = new QFontComboBox(optionsGroup);
    m_fontCombo->setCurrentFont(m_profile->defaultFont);
    auto *optionsLayout = new QGridLayout(optionsGroup);
    optionsLayout->addWidget(new QLabel(tr("Default Font")), 0, 0);
    optionsLayout->addWidget(m_fontCombo, 0, 1);
    optionsLayout->setColumnStretch(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(monitorGroup);
    layout->addWidget(marginsGroup);
    layout->addWidget(optionsGroup);
    layout->addStretch(1);

    connect(m_monitorCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_profile->monitorIndex = index - 1; // row 0 is "None"
        emit previewRelevantChanged();
    });
    connect(m_alphaCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_profile->alphaChannel = static_cast<OutputProfile::AlphaChannel>(index);
    });
    connect(m_posLeft, &QSpinBox::valueChanged, this, [this](int v) { m_profile->outputPosition.moveLeft(v); });
    connect(m_posTop, &QSpinBox::valueChanged, this, [this](int v) { m_profile->outputPosition.moveTop(v); });
    connect(m_posWidth, &QSpinBox::valueChanged, this, [this](int v) {
        m_profile->outputPosition.setWidth(v);
        emit previewRelevantChanged();
    });
    connect(m_posHeight, &QSpinBox::valueChanged, this, [this](int v) {
        m_profile->outputPosition.setHeight(v);
        emit previewRelevantChanged();
    });
    connect(m_marginLeft, &QSpinBox::valueChanged, this, [this](int v) { m_profile->marginLeft = v; });
    connect(m_marginTop, &QSpinBox::valueChanged, this, [this](int v) { m_profile->marginTop = v; });
    connect(m_marginRight, &QSpinBox::valueChanged, this, [this](int v) { m_profile->marginRight = v; });
    connect(m_marginBottom, &QSpinBox::valueChanged, this, [this](int v) { m_profile->marginBottom = v; });
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &f) { m_profile->defaultFont = f; });

    connect(m_monitorSetupButton, &QPushButton::clicked, this, [this]() {
        QStringList lines;
        const QList<QScreen *> screens = QGuiApplication::screens();
        for (int i = 0; i < screens.size(); ++i) {
            QScreen *screen = screens.at(i);
            lines << tr("Monitor %1: %2 -- %3x%4 at (%5, %6)")
                         .arg(i + 1)
                         .arg(screen->name())
                         .arg(screen->geometry().width())
                         .arg(screen->geometry().height())
                         .arg(screen->geometry().x())
                         .arg(screen->geometry().y());
        }
        QMessageBox::information(this, tr("Monitor Setup"),
            lines.isEmpty() ? tr("No monitors detected.") : lines.join('\n'));
    });
}

void OutputGeneralTab::populateMonitorCombo()
{
    m_monitorCombo->addItem(tr("None"));
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen *screen = screens.at(i);
        const bool isPrimary = (screen == QGuiApplication::primaryScreen());
        m_monitorCombo->addItem(tr("Monitor %1 (%2)%3")
                                     .arg(i + 1)
                                     .arg(isPrimary ? tr("Primary") : tr("Secondary"))
                                     .arg(QStringLiteral(" - %1x%2")
                                              .arg(screen->geometry().width())
                                              .arg(screen->geometry().height())));
    }
    // +1 because row 0 is the synthetic "None" entry.
    m_monitorCombo->setCurrentIndex(m_profile->monitorIndex + 1);
}

// ---------------------------------------------------------------------------
// OutputSongTab
// ---------------------------------------------------------------------------

OutputSongTab::OutputSongTab(OutputProfile *profile, QWidget *parent) : QWidget(parent), m_profile(profile)
{
    auto *group = new QGroupBox(tr("Song Slides"), this);
    auto *showBackground = new QCheckBox(tr("Show background behind lyrics"), group);
    showBackground->setChecked(m_profile->songShowBackground);
    auto *showTitle = new QCheckBox(tr("Show song title on first slide"), group);
    showTitle->setChecked(m_profile->songShowSongTitle);
    auto *maxLines = new QSpinBox(group);
    maxLines->setRange(1, 12);
    maxLines->setValue(m_profile->songMaxLinesPerSlide);

    auto *layout = new QGridLayout(group);
    layout->addWidget(showBackground, 0, 0, 1, 2);
    layout->addWidget(showTitle, 1, 0, 1, 2);
    layout->addWidget(new QLabel(tr("Max lines per slide")), 2, 0);
    layout->addWidget(maxLines, 2, 1);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(group);
    outer->addStretch(1);

    connect(showBackground, &QCheckBox::toggled, this, [this](bool on) { m_profile->songShowBackground = on; });
    connect(showTitle, &QCheckBox::toggled, this, [this](bool on) { m_profile->songShowSongTitle = on; });
    connect(maxLines, &QSpinBox::valueChanged, this, [this](int v) { m_profile->songMaxLinesPerSlide = v; });
}

// ---------------------------------------------------------------------------
// OutputScriptureTab
// ---------------------------------------------------------------------------

OutputScriptureTab::OutputScriptureTab(OutputProfile *profile, QWidget *parent) : QWidget(parent), m_profile(profile)
{
    auto *group = new QGroupBox(tr("Scripture Slides"), this);
    auto *showTranslation = new QCheckBox(tr("Show translation abbreviation"), group);
    showTranslation->setChecked(m_profile->scriptureShowTranslation);
    auto *showVerseNumbers = new QCheckBox(tr("Show verse numbers"), group);
    showVerseNumbers->setChecked(m_profile->scriptureShowVerseNumbers);
    auto *showRefEverySlide = new QCheckBox(tr("Show reference on every slide"), group);
    showRefEverySlide->setChecked(m_profile->scriptureShowReferenceOnEverySlide);

    auto *layout = new QVBoxLayout(group);
    layout->addWidget(showTranslation);
    layout->addWidget(showVerseNumbers);
    layout->addWidget(showRefEverySlide);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(group);
    outer->addStretch(1);

    connect(showTranslation, &QCheckBox::toggled, this, [this](bool on) { m_profile->scriptureShowTranslation = on; });
    connect(showVerseNumbers, &QCheckBox::toggled, this, [this](bool on) { m_profile->scriptureShowVerseNumbers = on; });
    connect(showRefEverySlide, &QCheckBox::toggled, this, [this](bool on) { m_profile->scriptureShowReferenceOnEverySlide = on; });
}

// ---------------------------------------------------------------------------
// OutputPresentationTab
// ---------------------------------------------------------------------------

OutputPresentationTab::OutputPresentationTab(OutputProfile *profile, QWidget *parent)
    : QWidget(parent), m_profile(profile)
{
    auto *group = new QGroupBox(tr("Presentations"), this);
    auto *clearAtEnd = new QCheckBox(tr("Clear output when a presentation ends"), group);
    clearAtEnd->setChecked(m_profile->presentationClearAtEnd);
    auto *loop = new QCheckBox(tr("Loop back to the first slide"), group);
    loop->setChecked(m_profile->presentationLoop);
    auto *speakerNotes = new QCheckBox(tr("Show speaker notes on the operator's screen only"), group);
    speakerNotes->setChecked(m_profile->presentationShowSpeakerNotesLocally);

    auto *layout = new QVBoxLayout(group);
    layout->addWidget(clearAtEnd);
    layout->addWidget(loop);
    layout->addWidget(speakerNotes);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(group);
    outer->addStretch(1);

    connect(clearAtEnd, &QCheckBox::toggled, this, [this](bool on) { m_profile->presentationClearAtEnd = on; });
    connect(loop, &QCheckBox::toggled, this, [this](bool on) { m_profile->presentationLoop = on; });
    connect(speakerNotes, &QCheckBox::toggled, this, [this](bool on) { m_profile->presentationShowSpeakerNotesLocally = on; });
}

// ---------------------------------------------------------------------------
// OutputTransitionsTab
// ---------------------------------------------------------------------------

OutputTransitionsTab::OutputTransitionsTab(OutputProfile *profile, QWidget *parent)
    : QWidget(parent), m_profile(profile)
{
    auto *group = new QGroupBox(tr("Slide Transitions"), this);
    auto *styleCombo = new QComboBox(group);
    styleCombo->addItem(OutputProfile::transitionStyleLabel(OutputProfile::TransitionStyle::Cut));
    styleCombo->addItem(OutputProfile::transitionStyleLabel(OutputProfile::TransitionStyle::Dissolve));
    styleCombo->addItem(OutputProfile::transitionStyleLabel(OutputProfile::TransitionStyle::Push));
    styleCombo->setCurrentIndex(static_cast<int>(m_profile->transitionStyle));

    auto *duration = new QSpinBox(group);
    duration->setRange(0, 5000);
    duration->setSingleStep(50);
    duration->setSuffix(tr(" ms"));
    duration->setValue(m_profile->transitionDurationMs);

    auto *layout = new QGridLayout(group);
    layout->addWidget(new QLabel(tr("Style")), 0, 0);
    layout->addWidget(styleCombo, 0, 1);
    layout->addWidget(new QLabel(tr("Duration")), 1, 0);
    layout->addWidget(duration, 1, 1);
    layout->setColumnStretch(1, 1);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(group);
    outer->addStretch(1);

    connect(styleCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_profile->transitionStyle = static_cast<OutputProfile::TransitionStyle>(index);
    });
    connect(duration, &QSpinBox::valueChanged, this, [this](int v) { m_profile->transitionDurationMs = v; });
}

// ---------------------------------------------------------------------------
// OutputAlertsTab
// ---------------------------------------------------------------------------

OutputAlertsTab::OutputAlertsTab(OutputProfile *profile, QWidget *parent) : QWidget(parent), m_profile(profile)
{
    auto *group = new QGroupBox(tr("On-Screen Alerts"), this);
    auto *enabled = new QCheckBox(tr("Allow alerts on this output"), group);
    enabled->setChecked(m_profile->alertsEnabled);
    auto *fontCombo = new QFontComboBox(group);
    fontCombo->setCurrentFont(m_profile->alertFont);
    auto *duration = new QSpinBox(group);
    duration->setRange(1, 120);
    duration->setSuffix(tr(" s"));
    duration->setValue(m_profile->alertDurationSeconds);

    auto *layout = new QGridLayout(group);
    layout->addWidget(enabled, 0, 0, 1, 2);
    layout->addWidget(new QLabel(tr("Alert Font")), 1, 0);
    layout->addWidget(fontCombo, 1, 1);
    layout->addWidget(new QLabel(tr("Duration")), 2, 0);
    layout->addWidget(duration, 2, 1);
    layout->setColumnStretch(1, 1);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(group);
    outer->addStretch(1);

    connect(enabled, &QCheckBox::toggled, this, [this, fontCombo, duration](bool on) {
        m_profile->alertsEnabled = on;
        fontCombo->setEnabled(on);
        duration->setEnabled(on);
    });
    fontCombo->setEnabled(m_profile->alertsEnabled);
    duration->setEnabled(m_profile->alertsEnabled);
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &f) { m_profile->alertFont = f; });
    connect(duration, &QSpinBox::valueChanged, this, [this](int v) { m_profile->alertDurationSeconds = v; });
}
