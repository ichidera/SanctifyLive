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
//
// PREVIEW vs LIVE: there are two independent cursors into m_slides.
// currentIndex/currentSlide() is what the congregation is seeing right
// now. previewIndex/previewSlide() is what the operator has staged --
// selecting something in the queue only moves the preview cursor, it
// never touches live output until goLive() is called explicitly. This
// mirrors how EasyWorship/ProPresenter avoid accidental live cuts: you
// can browse and stage the next several items while the wrong slide is
// still safely on screen. Fast keyboard/clicker navigation (advance,
// retreat, goToIndex) moves both cursors together, since that path is
// meant to be an immediate live cut.
class ScheduleModel : public QObject
{
    Q_OBJECT

public:
    explicit ScheduleModel(QObject *parent = nullptr);

    void addSlide(const Slide &slide);
    void removeSlideAt(int index);
    void clearAll(); // wipes the schedule and resets both cursors

    int count() const;
    const Slide &slideAt(int index) const;

    int currentIndex() const { return m_currentIndex; }
    int previewIndex() const { return m_previewIndex; }
    bool isBlackout() const { return m_blackout; }

    // Returns nullptr if there is no current slide (empty schedule or blackout).
    const Slide *currentSlide() const;
    const Slide *nextSlide() const;

    // Returns nullptr if nothing is staged. Deliberately NOT affected by
    // blackout -- the operator should be able to keep staging the next
    // item while live is blacked out.
    const Slide *previewSlide() const;

public slots:
    // Moves BOTH the live and preview cursor -- an immediate live cut.
    // Used by keyboard/clicker shortcuts, not by clicking in the queue.
    void goToIndex(int index);
    void advance();   // move to next slide (no-op if already at the end)
    void retreat();   // move to previous slide (no-op if already at the start)

    // Stages a slide for preview only. This is what clicking an item in
    // the queue should call -- it never touches the live output.
    void setPreviewIndex(int index);
    void clearPreview(); // deselects the staged item ("Clear" toolbar action)

    // Cuts live to whatever is currently staged in preview, and clears
    // blackout (going live should always show something).
    void goLive();

    void setBlackout(bool blackout);
    void toggleBlackout();

signals:
    // Fired whenever the schedule contents change (add/remove) so views
    // can refresh their lists.
    void scheduleChanged();

    // Fired whenever what should be on the LIVE output changes -- either
    // because the current index moved, or because blackout was toggled.
    // This is the one signal the LIVE OutputWindow needs to listen to.
    void liveContentChanged();

    // Fired whenever the STAGED item changes. This is the one signal the
    // PREVIEW OutputWindow needs to listen to -- it never listens to
    // liveContentChanged.
    void previewChanged();

private:
    QVector<Slide> m_slides;
    int m_currentIndex = -1; // -1 means "nothing live yet"
    int m_previewIndex = -1; // -1 means "nothing staged"
    bool m_blackout = false;
};
