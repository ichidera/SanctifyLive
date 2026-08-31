#include "ScheduleModel.h"

ScheduleModel::ScheduleModel(QObject *parent) : QObject(parent)
{
    // Whenever the main live content changes, "phone" clients need to
    // hear about it too, UNLESS an override is active -- in which case
    // the phone deliberately isn't following the main output right now,
    // and this change shouldn't disturb it.
    connect(this, &ScheduleModel::liveContentChanged, this, [this]() {
        if (!m_hasPhoneOverride)
            emit phoneContentChanged();
    });
}

// ---------------------------------------------------------------------
// Schedule editing
// ---------------------------------------------------------------------

void ScheduleModel::addSlide(const Slide &slide)
{
    m_slides.append(slide);
    // Deliberately does NOT touch live/cursor state -- adding something
    // to the schedule is just planning, not broadcasting.
    emit scheduleChanged();
}

void ScheduleModel::removeSlideAt(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;

    m_slides.removeAt(index);

    if (m_liveScheduleIndex == index) {
        // The item that was live got removed from the plan; the live
        // output keeps showing it (it's a copied value in m_liveSlide),
        // but it's no longer attributable to a schedule row.
        m_liveScheduleIndex = -1;
    } else if (m_liveScheduleIndex > index) {
        m_liveScheduleIndex--;
    }

    if (m_scheduleCursor >= m_slides.size())
        m_scheduleCursor = m_slides.size() - 1;
    else if (m_scheduleCursor > index)
        m_scheduleCursor--;

    emit scheduleChanged();
}

void ScheduleModel::clearAll()
{
    if (m_slides.isEmpty())
        return;

    m_slides.clear();
    m_scheduleCursor = -1;
    m_liveScheduleIndex = -1;
    // Live output and history are intentionally left alone -- clearing
    // the plan for a new service doesn't mean cutting whatever is
    // currently being shown.
    emit scheduleChanged();
}

int ScheduleModel::count() const
{
    return m_slides.size();
}

const Slide &ScheduleModel::slideAt(int index) const
{
    return m_slides.at(index);
}

// ---------------------------------------------------------------------
// Going live
// ---------------------------------------------------------------------

void ScheduleModel::goLiveFromSchedule(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;

    m_liveSlide = m_slides.at(index);
    m_hasLiveSlide = true;
    m_liveScheduleIndex = index;
    m_scheduleCursor = index;

    pushHistory(m_liveSlide);
    emit liveContentChanged();
}

void ScheduleModel::sendMediaLive(const Slide &slide)
{
    if (m_autoAddMediaToSchedule) {
        addSlide(slide);
        m_liveScheduleIndex = m_slides.size() - 1;
        m_scheduleCursor = m_liveScheduleIndex;
    } else {
        m_liveScheduleIndex = -1;
    }

    m_liveSlide = slide;
    m_hasLiveSlide = true;

    pushHistory(m_liveSlide);
    emit liveContentChanged();
}

void ScheduleModel::goLiveFromHistoryAt(int index)
{
    if (index < 0 || index >= m_history.size())
        return;

    m_liveSlide = m_history.at(index);
    m_hasLiveSlide = true;
    m_liveScheduleIndex = -1; // history doesn't track schedule provenance

    pushHistory(m_liveSlide); // re-promote to the front
    emit liveContentChanged();
}

void ScheduleModel::advanceLive()
{
    if (m_scheduleCursor + 1 < m_slides.size())
        goLiveFromSchedule(m_scheduleCursor + 1);
}

void ScheduleModel::retreatLive()
{
    if (m_scheduleCursor - 1 >= 0)
        goLiveFromSchedule(m_scheduleCursor - 1);
}

void ScheduleModel::setBlackout(bool blackout)
{
    if (m_blackout == blackout)
        return;
    m_blackout = blackout;
    emit liveContentChanged();
}

void ScheduleModel::toggleBlackout()
{
    setBlackout(!m_blackout);
}

const Slide *ScheduleModel::liveSlide() const
{
    if (m_blackout || !m_hasLiveSlide)
        return nullptr;
    return &m_liveSlide;
}

const Slide *ScheduleModel::previewSlide() const
{
    if (!m_hasLiveSlide)
        return nullptr;
    return &m_liveSlide;
}

// ---------------------------------------------------------------------
// Phone-only content override
// ---------------------------------------------------------------------

void ScheduleModel::sendPhoneOverride(const Slide &slide)
{
    m_phoneOverrideSlide = slide;
    m_hasPhoneOverride = true;
    emit phoneContentChanged();
}

void ScheduleModel::clearPhoneOverride()
{
    if (!m_hasPhoneOverride)
        return; // avoid a spurious push when there was nothing to clear
    m_hasPhoneOverride = false;
    emit phoneContentChanged();
}

const Slide *ScheduleModel::phoneSlide() const
{
    if (m_hasPhoneOverride)
        return &m_phoneOverrideSlide;
    return liveSlide();
}

// ---------------------------------------------------------------------
// History
// ---------------------------------------------------------------------

int ScheduleModel::historyCount() const
{
    return m_history.size();
}

const Slide &ScheduleModel::historyAt(int index) const
{
    return m_history.at(index);
}

void ScheduleModel::pushHistory(const Slide &slide)
{
    // De-dupe by label so repeatedly re-showing the same item doesn't
    // spam the strip with copies -- move it to the front instead.
    for (int i = 0; i < m_history.size(); ++i) {
        if (m_history.at(i).label == slide.label) {
            m_history.removeAt(i);
            break;
        }
    }

    m_history.prepend(slide);
    while (m_history.size() > kMaxHistory)
        m_history.removeLast();

    emit historyChanged();
}