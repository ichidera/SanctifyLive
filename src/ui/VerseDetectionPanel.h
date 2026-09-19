#pragma once

#include <QVector>
#include <QWidget>

#include "core/scripture/ScriptureDetector.h"

class QLabel;
class QListWidget;
class QListWidgetItem;

// VerseDetectionPanel shows the Bible references ScriptureDetector
// found in a piece of transcribed text (see
// core/scripture/ScriptureDetector.h), each as a row an operator can
// click to preview or double-click to send straight to Schedule -- the
// on-screen half of "notice a reference was spoken and offer to put it
// up", the feature real-time verse-detection tools like Pewbeam build
// their whole product around.
//
// This panel does not do any detection itself and does not know where
// transcribed text comes from -- showDetections() is the entire surface
// OperatorWindow calls into after running ScriptureDetector against
// whatever text just arrived. Today that's only ever a manually typed
// test line (see OperatorWindow's "Simulate Caption" control) -- there
// is still no real speech-to-text engine anywhere in this project (see
// README roadmap) -- but the moment one exists and starts calling
// LiveCaptionsPanel::appendCaption(), the exact same detection ->
// display -> click-to-add path already works, unchanged.
//
// Kept as its own class rather than folded into LiveCaptionsPanel
// because a raw transcript log and "here's what we found in it, click
// to act" are genuinely different kinds of content an operator reads
// differently -- one is "what was said", the other is "what to do
// about it now".
class VerseDetectionPanel : public QWidget
{
    Q_OBJECT

public:
    explicit VerseDetectionPanel(QWidget *parent = nullptr);

    // Replaces whatever was shown with these detections -- this is "what
    // was just found in the latest text", not an accumulating log, so a
    // scan that finds nothing clears the list back to the placeholder
    // rather than leaving stale results from an earlier line on screen.
    void showDetections(const QVector<ScriptureDetector::Detection> &detections);

signals:
    void detectionActivated(const QString &reference, const QString &text);
    void detectionPreviewRequested(const QString &reference, const QString &text);

private:
    QLabel *m_placeholderLabel = nullptr;
    QListWidget *m_list = nullptr;
};
