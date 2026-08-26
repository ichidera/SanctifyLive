#include "ScheduleModel.h"

ScheduleModel::ScheduleModel(QObject *parent) : QObject(parent)
{
}

void ScheduleModel::addSlide(const Slide &slide)
{
    m_slides.append(slide);
    if (m_liveIndex == -1) {
        m_liveIndex = 0;
        m_previewIndex = 0;
        emit liveContentChanged();
        emit previewChanged();
    }
    emit scheduleChanged();
}

void ScheduleModel::removeSlideAt(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;

    m_slides.removeAt(index);

    if (m_slides.isEmpty()) {
        m_liveIndex = -1;
        m_previewIndex = -1;
    } else {
        if (m_liveIndex >= m_slides.size())
            m_liveIndex = m_slides.size() - 1;
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
    m_liveIndex = -1;
    m_previewIndex = -1;

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

const Slide *ScheduleModel::liveSlide() const
{
    if (m_blackout)
        return nullptr;
    if (m_liveIndex < 0 || m_liveIndex >= m_slides.size())
        return nullptr;
    return &m_slides.at(m_liveIndex);
}

const Slide *ScheduleModel::previewSlide() const
{
    if (m_previewIndex < 0 || m_previewIndex >= m_slides.size())
        return nullptr;
    return &m_slides.at(m_previewIndex);
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

void ScheduleModel::goLiveWithPreview()
{
    if (m_previewIndex < 0 || m_previewIndex >= m_slides.size())
        return;
    if (m_previewIndex == m_liveIndex)
        return;

    m_liveIndex = m_previewIndex;
    emit liveContentChanged();
}

void ScheduleModel::advanceLive()
{
    if (m_liveIndex + 1 >= m_slides.size())
        return;

    m_liveIndex++;
    emit liveContentChanged();

    if (m_previewIndex != m_liveIndex) {
        m_previewIndex = m_liveIndex;
        emit previewChanged();
    }
}

void ScheduleModel::retreatLive()
{
    if (m_liveIndex - 1 < 0)
        return;

    m_liveIndex--;
    emit liveContentChanged();

    if (m_previewIndex != m_liveIndex) {
        m_previewIndex = m_liveIndex;
        emit previewChanged();
    }
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