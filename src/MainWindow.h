#pragma once
//
// MainWindow.h
// Assembles the whole SanctifyLive-style layout from the individual panels,
// wiring everything together with nested QSplitters so every region of the
// interface is user-resizable.
//
#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
};
