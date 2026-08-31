#ifndef SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTCONTENTTABS_H_
#define SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTCONTENTTABS_H_

#include <QWidget>

#include "../OutputProfile.h"

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QSpinBox;
class QPushButton;
class QGroupBox;

// Six small widgets, one per tab of OutputSettingsPage's QTabWidget
// (General / Song / Scripture / Presentation / Transitions / Alerts).
// Grouped in one file because each is only a couple of controls wide --
// splitting them into six header/source pairs would just be ceremony --
// but they are still six independent classes, each owning exactly the
// slice of OutputProfile it edits. That's what keeps this "modular" in
// the sense that matters: OutputSettingsPage can be reordered, and any
// one tab can be dropped, reused standalone, or grown into its own file
// later, without touching the others.
//
// Common contract for all six: constructed with the OutputProfile this
// destination owns, immediately reflect that profile's current values,
// and write straight back into the profile as the operator edits --
// there is no separate "apply" step per field. SettingsWindow is the one
// that decides when a *whole* profile's edits are kept (OK) or thrown
// away (Cancel), by snapshotting/restoring the OutputProfile itself; see
// SettingsWindow.cpp.

class OutputGeneralTab : public QWidget
{
    Q_OBJECT
public:
    explicit OutputGeneralTab(OutputProfile *profile, QWidget *parent = nullptr);

signals:
    // So the shared preview thumbnail (owned by SettingsWindow, not this
    // tab) can be kept in sync without this tab needing to know it exists.
    void previewRelevantChanged();

private:
    OutputProfile *m_profile;
    QComboBox *m_monitorCombo;
    QComboBox *m_alphaCombo;
    QSpinBox *m_posLeft;
    QSpinBox *m_posTop;
    QSpinBox *m_posWidth;
    QSpinBox *m_posHeight;
    QSpinBox *m_marginLeft;
    QSpinBox *m_marginTop;
    QSpinBox *m_marginRight;
    QSpinBox *m_marginBottom;
    QFontComboBox *m_fontCombo;
    QPushButton *m_monitorSetupButton;

    void populateMonitorCombo();
};

class OutputSongTab : public QWidget
{
    Q_OBJECT
public:
    explicit OutputSongTab(OutputProfile *profile, QWidget *parent = nullptr);

private:
    OutputProfile *m_profile;
};

class OutputScriptureTab : public QWidget
{
    Q_OBJECT
public:
    explicit OutputScriptureTab(OutputProfile *profile, QWidget *parent = nullptr);

private:
    OutputProfile *m_profile;
};

class OutputPresentationTab : public QWidget
{
    Q_OBJECT
public:
    explicit OutputPresentationTab(OutputProfile *profile, QWidget *parent = nullptr);

private:
    OutputProfile *m_profile;
};

class OutputTransitionsTab : public QWidget
{
    Q_OBJECT
public:
    explicit OutputTransitionsTab(OutputProfile *profile, QWidget *parent = nullptr);

private:
    OutputProfile *m_profile;
};

class OutputAlertsTab : public QWidget
{
    Q_OBJECT
public:
    explicit OutputAlertsTab(OutputProfile *profile, QWidget *parent = nullptr);

private:
    OutputProfile *m_profile;
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTCONTENTTABS_H_
