#ifndef SANCTIFYLIVE_CORE_SETTINGS_OUTPUTPROFILE_H_
#define SANCTIFYLIVE_CORE_SETTINGS_OUTPUTPROFILE_H_

#include <QFont>
#include <QObject>
#include <QRect>

// Plain-data settings for one output destination. SanctifyLive has four
// destinations that all need the exact same shape of configuration --
// Main Output, Alternate Output, Foldback, and the Android stage-display
// link -- so rather than four hand-written pages, there is one
// OutputProfile struct and one OutputSettingsPage (see
// pages/OutputSettingsPage.h) that reads/writes whichever profile it's
// pointed at. Adding a fifth output later is a matter of constructing
// another OutputProfile and another OutputSettingsPage, not writing new
// UI.
//
// This struct is intentionally backend-agnostic: it does not know about
// QScreen, OutputWindow, or the network link. SettingsWindow is the only
// place that translates a profile's monitorIndex into an actual
// QGuiApplication::screens() entry. That keeps this file safe to unit
// test and safe to reuse for a destination (Android) that has no QScreen
// at all.
struct OutputProfile
{
    enum class AlphaChannel { Disabled, Straight, Premultiplied };
    enum class TransitionStyle { Cut, Dissolve, Push };

    QString displayName; // "Main Output", "Alternate Output", "Foldback", "Android"

    // --- General tab: which screen, and how it's framed ---
    int monitorIndex = -1; // index into QGuiApplication::screens(); -1 = "None"
    AlphaChannel alphaChannel = AlphaChannel::Disabled;
    QRect outputPosition{0, 0, 1920, 1080}; // Left/Top/Width/Height
    int marginLeft = 0;
    int marginTop = 0;
    int marginRight = 0;
    int marginBottom = 0;
    QFont defaultFont;

    // --- Song tab ---
    bool songShowBackground = true;
    int songMaxLinesPerSlide = 4;
    bool songShowSongTitle = false;

    // --- Scripture tab ---
    bool scriptureShowTranslation = true;
    bool scriptureShowVerseNumbers = false;
    bool scriptureShowReferenceOnEverySlide = true;

    // --- Presentation tab ---
    bool presentationClearAtEnd = true;
    bool presentationLoop = false;
    bool presentationShowSpeakerNotesLocally = false;

    // --- Transitions tab ---
    TransitionStyle transitionStyle = TransitionStyle::Dissolve;
    int transitionDurationMs = 500;

    // --- Alerts tab ---
    bool alertsEnabled = true;
    QFont alertFont;
    int alertDurationSeconds = 8;

    static QString alphaChannelLabel(AlphaChannel value)
    {
        switch (value) {
        case AlphaChannel::Disabled: return QObject::tr("Disabled");
        case AlphaChannel::Straight: return QObject::tr("Straight");
        case AlphaChannel::Premultiplied: return QObject::tr("Premultiplied");
        }
        return {};
    }

    static QString transitionStyleLabel(TransitionStyle value)
    {
        switch (value) {
        case TransitionStyle::Cut: return QObject::tr("Cut");
        case TransitionStyle::Dissolve: return QObject::tr("Dissolve");
        case TransitionStyle::Push: return QObject::tr("Push");
        }
        return {};
    }
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_OUTPUTPROFILE_H_
