#pragma once
//
// QueuePanel.h
// The "Queue" list of upcoming slide cards, highlighted at the top.
//
#include <QWidget>

class QListWidget;

class QueuePanel : public QWidget
{
    Q_OBJECT
public:
    explicit QueuePanel(QWidget* parent = nullptr);

private:
    QListWidget* m_list = nullptr;
    void populateSampleData();
};
