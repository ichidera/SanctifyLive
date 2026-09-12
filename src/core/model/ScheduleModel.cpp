#include "core/model/ScheduleModel.h"

ScheduleModel::ScheduleModel(QObject *parent) : QObject(parent)
{
}

void ScheduleModel::addSlide(const Slide &slide)
{
    m_slides.append(slide);
    if (m_currentIndex == -1) {
        m_currentIndex = 0;
        recordHistory(m_currentIndex);
    }
    emit scheduleChanged();
    emit liveContentChanged();
}

void ScheduleModel::removeSlideAt(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;

    m_slides.removeAt(index);

    if (m_slides.isEmpty()) {
        m_currentIndex = -1;
    } else if (m_currentIndex >= m_slides.size()) {
        m_currentIndex = m_slides.size() - 1;
    }

    emit scheduleChanged();
    emit liveContentChanged();
}

void ScheduleModel::setSlides(const QVector<Slide> &slides)
{
    m_slides = slides;
    m_currentIndex = m_slides.isEmpty() ? -1 : 0;
    m_blackout = false;
    m_history.clear();
    if (m_currentIndex == 0)
        recordHistory(m_currentIndex);
    emit scheduleChanged();
    emit liveContentChanged();
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

void ScheduleModel::goToIndex(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;
    if (index == m_currentIndex)
        return;

    m_currentIndex = index;
    recordHistory(m_currentIndex);
    emit liveContentChanged();
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

void ScheduleModel::recordHistory(int index)
{
    if (index < 0 || index >= m_slides.size())
        return;

    HistoryEntry entry;
    entry.slideIndex = index;
    entry.label = m_slides.at(index).label;
    entry.timestamp = QDateTime::currentDateTime();
    m_history.append(entry);
    emit historyChanged();
}
