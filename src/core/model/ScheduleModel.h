#pragma once

#include <QObject>
#include <QVector>

#include "core/model/Slide.h"

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

private:
    QVector<Slide> m_slides;
    int m_currentIndex = -1; // -1 means "nothing selected yet"
    bool m_blackout = false;
};
