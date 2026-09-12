#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

#include "core/model/Slide.h"

// One entry in the "what has actually gone live, in order" log. This is
// deliberately separate from the schedule's slide list: the schedule is
// the *plan* (and can be reordered/edited), while history is an
// append-only record of what the congregation was actually shown and
// when -- useful for an operator reviewing "what did we just show" or
// reconstructing a run of the service after the fact.
struct HistoryEntry
{
    int slideIndex = -1;
    QString label;
    QDateTime timestamp;
};

// ScheduleModel is the single source of truth for "what is the service
// doing right now." Both the OperatorWindow (control surface) and the
// OutputWindow (what the congregation sees) observe this model rather
// than talking to each other directly -- that separation is what lets
// the output stay correct no matter how many control surfaces exist
// later (in-app operator, web remote, stage view, etc.).
//
// This is intentionally a flat list of Slides for the first milestone.
// Grouping slides into higher-level "schedule items" (a whole song, a
// whole Scripture reading) is a near-term follow-up once this loop is
// proven live.
class ScheduleModel : public QObject
{
    Q_OBJECT

public:
    explicit ScheduleModel(QObject *parent = nullptr);

    void addSlide(const Slide &slide);
    void removeSlideAt(int index);

    int count() const;
    const Slide &slideAt(int index) const;

    int currentIndex() const { return m_currentIndex; }
    bool isBlackout() const { return m_blackout; }

    // Returns nullptr if there is no current slide (empty schedule or blackout).
    const Slide *currentSlide() const;
    const Slide *nextSlide() const;

    // Read-only access to the full slide list, e.g. for serialization
    // (see ScheduleIO) or for UI panels that need to browse everything
    // rather than just the live/next pair.
    const QVector<Slide> &slides() const { return m_slides; }

    // Append-only log of slides that have actually gone live, oldest
    // first. Populated automatically every time the live position moves
    // (see goToIndex()) -- nothing else needs to maintain this.
    const QVector<HistoryEntry> &history() const { return m_history; }

    // Wholesale replace of the schedule contents (e.g. after loading a
    // saved schedule from disk via ScheduleIO). Resets live position to
    // the first slide and clears blackout, same as starting fresh.
    void setSlides(const QVector<Slide> &slides);

public slots:
    void goToIndex(int index);
    void advance();   // move to next slide (no-op if already at the end)
    void retreat();   // move to previous slide (no-op if already at the start)
    void setBlackout(bool blackout);
    void toggleBlackout();

signals:
    // Fired whenever the schedule contents change (add/remove) so views
    // can refresh their lists.
    void scheduleChanged();

    // Fired whenever what should be on the LIVE output changes -- either
    // because the current index moved, or because blackout was toggled.
    // This is the one signal OutputWindow needs to listen to.
    void liveContentChanged();

    // Fired whenever a new entry is appended to history() -- e.g. so a
    // History panel can append a row without rebuilding its whole list.
    void historyChanged();

private:
    void recordHistory(int index);

    QVector<Slide> m_slides;
    int m_currentIndex = -1; // -1 means "nothing selected yet"
    bool m_blackout = false;
    QVector<HistoryEntry> m_history;
};
