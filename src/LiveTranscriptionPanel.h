#pragma once
//
// LiveTranscriptionPanel.h
// Left-hand "Live transcription" list - scrolling, color-coded verse feed.
//
#include <QWidget>

class QListWidget;

class LiveTranscriptionPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LiveTranscriptionPanel(QWidget* parent = nullptr);

private:
    QListWidget* m_list = nullptr;
    void populateSampleData();
};
