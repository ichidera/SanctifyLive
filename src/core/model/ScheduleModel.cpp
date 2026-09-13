#include "core/model/ScheduleModel.h"

ScheduleModel::ScheduleModel(QObject *parent) : QObject(parent)
{
}

void ScheduleModel::addSlide(const Slide &slide)
{
    m_slides.append(slide);
    if (m_currentIndex == -1) {
        m_currentIndex = 0;
        m_previewIndex = 0;
    }
    emit scheduleChanged();
    emit liveContentChanged();
    emit previewChanged();
}

void ScheduleModel::removeSlideAt(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;

    m_slides.removeAt(index);

    if (m_slides.isEmpty()) {
        m_currentIndex = -1;
        m_previewIndex = -1;
    } else {
        if (m_currentIndex >= m_slides.size())
            m_currentIndex = m_slides.size() - 1;
        if (m_previewIndex >= m_slides.size())
            m_previewIndex = m_slides.size() - 1;
    }

    emit scheduleChanged();
    emit liveContentChanged();
    emit previewChanged();
}

void ScheduleModel::clearAll()
{
    if (m_slides.isEmpty())
        return;

    m_slides.clear();
    m_currentIndex = -1;
    m_previewIndex = -1;
    m_blackout = false;

    emit scheduleChanged();
    emit liveContentChanged();
    emit previewChanged();
}

int ScheduleModel::count() const
{
    return m_slides.size();
}

const Slide &ScheduleModel::slideAt(int index) const
{
    return m_slides.at(index);
}

const Slide *ScheduleModel::currentSlide() const
{
    if (m_blackout)
        return nullptr;
    if (m_currentIndex < 0 || m_currentIndex >= m_slides.size())
        return nullptr;
    return &m_slides.at(m_currentIndex);
}

const Slide *ScheduleModel::nextSlide() const
{
    const int next = m_currentIndex + 1;
    if (next < 0 || next >= m_slides.size())
        return nullptr;
    return &m_slides.at(next);
}

const Slide *ScheduleModel::previewSlide() const
{
    // Deliberately does NOT check m_blackout -- staging the next item
    // must keep working while live is blacked out.
    if (m_previewIndex < 0 || m_previewIndex >= m_slides.size())
        return nullptr;
    return &m_slides.at(m_previewIndex);
}

void ScheduleModel::goToIndex(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;

    // This is the "immediate live cut" path -- both cursors move
    // together so preview never lags behind a keyboard/clicker jump.
    const bool liveChanged = (index != m_currentIndex);
    const bool previewChangedFlag = (index != m_previewIndex);

    m_currentIndex = index;
    m_previewIndex = index;

    if (liveChanged)
        emit liveContentChanged();
    if (previewChangedFlag)
        emit previewChanged();
}

void ScheduleModel::advance()
{
    if (m_currentIndex + 1 < m_slides.size()) {
        goToIndex(m_currentIndex + 1);
    }
}

void ScheduleModel::retreat()
{
    if (m_currentIndex - 1 >= 0) {
        goToIndex(m_currentIndex - 1);
    }
}

void ScheduleModel::setPreviewIndex(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;
    if (index == m_previewIndex)
        return;

    m_previewIndex = index;
    emit previewChanged();
}

void ScheduleModel::clearPreview()
{
    if (m_previewIndex == -1)
        return;

    m_previewIndex = -1;
    emit previewChanged();
}

void ScheduleModel::goLive()
{
    if (m_previewIndex < 0 || m_previewIndex >= m_slides.size())
        return;

    // Going live should always actually show something, so drop
    // blackout unconditionally -- an operator who clicks "Go Live"
    // never wants to end up still staring at black.
    const bool wasBlackout = m_blackout;
    const bool indexChanged = (m_previewIndex != m_currentIndex);

    m_blackout = false;
    m_currentIndex = m_previewIndex;

    if (indexChanged || wasBlackout)
        emit liveContentChanged();
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
