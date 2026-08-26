#ifndef SANCTIFYLIVE_CORE_SCHEDULEMODEL_H_
#define SANCTIFYLIVE_CORE_SCHEDULEMODEL_H_


#include <QObject>
#include <QVector>

#include "Slide.h"

// ScheduleModel separates three ideas that were previously conflated:
//
//   - SCHEDULE: the ordered, pre-planned list of items for the service.
//     Nothing here changes just because something went live.
//   - LIVE: the actual content on the output right now. It's held as a
//     real Slide VALUE (not an index into the schedule), because
//     content can go live without ever being added to the schedule --
//     e.g. reaching into the media library for a one-off background.
//   - HISTORY: a capped, most-recent-first record of everything that
//     has been sent live, regardless of whether it came from the
//     schedule or the media library. This is what lets an operator
//     return to something they showed earlier without it having
//     cluttered the schedule.
//
// Next/Previous still walks the schedule in order (m_scheduleCursor)
// since that's the natural transport control during a service; sending
// something live from the media library doesn't disturb that cursor's
// position, it just becomes the new live content on top of it.
class ScheduleModel : public QObject
{
    Q_OBJECT

public:
    explicit ScheduleModel(QObject *parent = nullptr);

    // --- Schedule editing ---
    void addSlide(const Slide &slide);
    void removeSlideAt(int index);
    void clearAll();
    int count() const;
    const Slide &slideAt(int index) const;

    // --- Sending things live ---
    void goLiveFromSchedule(int index);
    void sendMediaLive(const Slide &slide);
    void goLiveFromHistoryAt(int index);
    void advanceLive(); // steps the schedule cursor forward and goes live with it
    void retreatLive();

    void setBlackout(bool blackout);
    void toggleBlackout();
    bool isBlackout() const { return m_blackout; }

    // Returns nullptr if there's no live content, OR if blacked out.
    const Slide *liveSlide() const;
    // Returns nullptr only if there's no live content. Ignores blackout,
    // so an operator can always see what's actually loaded even while
    // the audience screen is black.
    const Slide *previewSlide() const;

    // -1 if the current live content did not come from the schedule
    // (e.g. it was sent directly from the media library).
    int liveScheduleIndex() const { return m_liveScheduleIndex; }
    // Where Next/Previous will resume from; -1 if never navigated yet.
    int scheduleCursor() const { return m_scheduleCursor; }

    // --- History ---
    int historyCount() const;
    const Slide &historyAt(int index) const;

    // --- Options (Edit > Options) ---
    bool autoAddMediaToSchedule() const { return m_autoAddMediaToSchedule; }
    void setAutoAddMediaToSchedule(bool enabled) { m_autoAddMediaToSchedule = enabled; }

signals:
    void scheduleChanged();
    void liveContentChanged();
    void historyChanged();

private:
    void pushHistory(const Slide &slide);

    QVector<Slide> m_slides;       // the schedule
    int m_scheduleCursor = -1;     // Next/Previous position

    Slide m_liveSlide;
    bool m_hasLiveSlide = false;
    int m_liveScheduleIndex = -1;  // which schedule row (if any) is live
    bool m_blackout = false;

    QVector<Slide> m_history;      // most-recent-first
    bool m_autoAddMediaToSchedule = false;

    static constexpr int kMaxHistory = 16;
};

#endif // SANCTIFYLIVE_CORE_SCHEDULEMODEL_H_