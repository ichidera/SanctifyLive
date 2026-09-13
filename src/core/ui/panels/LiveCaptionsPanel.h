#pragma once

#include <QWidget>

// LiveCaptionsPanel is a placeholder for the live speech-to-text feed
// shown in the design mockup. There is no speech-recognition backend in
// this codebase yet -- rather than fabricate scrolling caption text (which
// would misrepresent what the app can currently do), this panel honestly
// shows a "not connected" state. Once a real transcription source exists,
// this class is the one place that needs to grow an actual feed: it can
// gain a method like appendCaption(const QString&) and everything else
// (layout, styling, where it sits in OperatorWindow) stays the same.
class LiveCaptionsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit LiveCaptionsPanel(QWidget *parent = nullptr);
};
