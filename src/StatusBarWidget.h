#pragma once
//
// StatusBarWidget.h
// Bottom strip: stage display connection info (left) and "Wake Display"
// action (right).
//
#include <QWidget>

class StatusBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBarWidget(QWidget* parent = nullptr);
};
