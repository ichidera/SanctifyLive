#pragma once

#include <QWidget>

class QPlainTextEdit;

// LiveCaptionsPanel is the display surface for the TRANSCRIPTION panel
// (see OperatorWindow's panel glossary): a scrolling, read-only log of
// what's been spoken, one caption at a time, always auto-scrolled to
// the newest line -- the same behavior as live captions/subtitles
// anywhere else.
//
// This is *just* the display. There's still no audio capture or
// speech-to-text engine anywhere in this project (see README roadmap).
// appendCaption() is the entire surface a future STT pipeline would
// call into: feed it finalized text and it shows up here. Until
// something calls that, the panel shows an honest placeholder rather
// than pretending to be listening -- same "don't fake it" philosophy as
// the disabled Start Transcription button next to it in
// OperatorWindow::buildTranscriptionPanel().
//
// Deliberately NOT guessing at interim/partial-result handling, speaker
// labels, timestamps, or export -- those are real features ("we'd add
// other features"), and baking in an API shape for them now, before
// there's an actual engine to drive it, would just be guessing at what
// that engine needs.
class LiveCaptionsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LiveCaptionsPanel(QWidget *parent = nullptr);

public slots:
    // Appends one finalized line of transcribed speech and scrolls it
    // into view. Empty/whitespace-only text is ignored rather than
    // adding a blank line -- a defensive no-op for whatever eventually
    // calls this from a real STT pipeline.
    void appendCaption(const QString &text);

    // Clears the transcript log back to the placeholder. Its own slot
    // (rather than folded into e.g. a future "stop" action) since
    // clearing and stopping are different operations a real Start/Stop
    // Transcription control would want independently.
    void clear();

private:
    // Caps how many caption lines are kept on screen. A multi-hour
    // service produces a lot of speech; without a cap this would grow
    // for the entire length of a service for no benefit -- there's no
    // "transcript archive" feature yet (unlike History, which is a
    // deliberate append-only record of slides), so nothing reads this
    // log back once it's scrolled out of view.
    static constexpr int kMaxLines = 500;

    QPlainTextEdit *m_view = nullptr;
};
