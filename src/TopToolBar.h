#pragma once
//
// TopToolBar.h
// The strip across the very top: brand mark, file-style actions (New, Open,
// Save, Store, Web, Remote), and the right-aligned Go Live / Alerts / Logo /
// Black / Clear / Live cluster. Pure UI - no signal wiring beyond stubs.
//
#include <QWidget>

class QToolButton;
class QPushButton;

class TopToolBar : public QWidget
{
    Q_OBJECT
public:
    explicit TopToolBar(QWidget* parent = nullptr);

private:
    QToolButton* addAction(QWidget* container, const QString& icon, const QString& label);
    QPushButton* addStatePill(QWidget* container, const QString& text, bool filled);
};
