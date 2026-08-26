#pragma once

#include <QObject>
#include <QVector>

#include "Slide.h"

// ScheduleModel is the single source of truth for the service, and it
// deliberately separates two ideas that are easy to conflate:
//
//   - PREVIEW: the slide the operator is currently looking at / staging.
//     Selecting an item in the schedule list only changes this. Nothing
//     the congregation sees changes when preview changes.
//   - LIVE: the slide actually on the output screen right now. Only
//     an explicit action (Go Live, double-click, Enter, or the
//     Next/Previous transport buttons) changes this.
//
// This mirrors EasyWorship/ProPresenter's preview-vs-live model, and it
// exists for a concrete HCI reason: an operator scanning ahead in the
// schedule (e.g. during a lull) must never accidentally push something
// to the screen just by clicking to look at it.
class ScheduleModel : public QObject
{
    Q_OBJECT

public:
    explicit ScheduleModel(QObject *parent = nullptr);

    void addSlide(const Slide &slide);
    void removeSlideAt(int index);
    void clearAll();

    int count() const;
    const Slide &slideAt(int index) const;

    int liveIndex() const { return m_liveIndex; }
    int previewIndex() const { return m_previewIndex; }
    bool isBlackout() const { return m_blackout; }

    // Returns nullptr if there is no live slide (empty schedule or blackout).
    const Slide *liveSlide() const;

    // Returns nullptr only if the schedule is empty. Blackout does NOT
    // affect this -- the operator should always be able to see what
    // they're about to send live, even while the audience screen is black.
    const Slide *previewSlide() const;

public slots:
    // Stages a slide for preview without touching what's live.
    void setPreviewIndex(int index);

    // Commits the currently-previewed slide to the live output.
    void goLiveWithPreview();

    // Direct live transport, for the Next/Previous buttons and keyboard
    // shortcuts used during the flow of a service. Keeps preview in sync
    // with live afterward so the schedule list highlighting stays sane.
    void advanceLive();
    void retreatLive();

    void setBlackout(bool blackout);
    void toggleBlackout();

signals:
    // Fired whenever the schedule contents change (add/remove) so views
    // can refresh their lists.
    void scheduleChanged();

    // Fired whenever the LIVE output should change -- index moved or
    // blackout toggled. This is what OutputWindow (in Live mode) listens to.
    void liveContentChanged();

    // Fired whenever the PREVIEW selection changes. This is what the
    // preview pane (OutputWindow in Preview mode) listens to.
    void previewChanged();

private:
    QVector<Slide> m_slides;
    int m_liveIndex = -1;
    int m_previewIndex = -1;
    bool m_blackout = false;
};