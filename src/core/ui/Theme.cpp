#include "core/ui/Theme.h"

#include <QApplication>
#include <QPalette>
#include <QString>

namespace Theme
{

void applyDark(QApplication &app)
{
    app.setStyle("Fusion");

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(Colors::background));
    pal.setColor(QPalette::WindowText, QColor(Colors::textPrimary));
    pal.setColor(QPalette::Base, QColor(Colors::panelAlt));
    pal.setColor(QPalette::AlternateBase, QColor(Colors::panel));
    pal.setColor(QPalette::Text, QColor(Colors::textPrimary));
    pal.setColor(QPalette::Button, QColor(Colors::panel));
    pal.setColor(QPalette::ButtonText, QColor(Colors::textPrimary));
    pal.setColor(QPalette::Highlight, QColor(Colors::accentSelect));
    pal.setColor(QPalette::HighlightedText, QColor(Colors::textPrimary));
    pal.setColor(QPalette::PlaceholderText, QColor(Colors::textMuted));
    pal.setColor(QPalette::ToolTipBase, QColor(Colors::panelAlt));
    pal.setColor(QPalette::ToolTipText, QColor(Colors::textPrimary));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(Colors::textMuted));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(Colors::textMuted));
    app.setPalette(pal);

    // A thin QSS layer on top of the palette for the things a QPalette
    // can't express: rounded panel corners, border colors, hover states,
    // and the checked/"live" look for toggle buttons. Every color here
    // is pulled from Theme::Colors so there's a single source of truth.
    const QString qss = QString(R"(
        QMainWindow, QWidget {
            background-color: %1;
            color: %2;
        }
        QToolBar {
            background-color: %3;
            border: none;
            border-bottom: 1px solid %4;
            spacing: 4px;
            padding: 4px 8px;
        }
        QToolButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            padding: 6px 10px;
            color: %2;
        }
        QToolButton:hover {
            background-color: %5;
        }
        QToolButton:checked {
            background-color: %6;
            color: white;
        }
        QGroupBox {
            border: 1px solid %4;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 8px;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: %7;
        }
        QTabWidget::pane {
            border: 1px solid %4;
            border-radius: 6px;
            top: -1px;
        }
        QTabBar::tab {
            background: %3;
            color: %7;
            padding: 6px 14px;
            border: 1px solid %4;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QTabBar::tab:selected {
            background: %5;
            color: %2;
        }
        QListWidget, QListView, QTreeView {
            background-color: %5;
            border: 1px solid %4;
            border-radius: 6px;
            outline: none;
        }
        QListWidget::item, QListView::item {
            padding: 6px;
            border-radius: 4px;
        }
        QListWidget::item:selected, QListView::item:selected {
            background-color: %6;
            color: white;
        }
        QPushButton {
            background-color: %5;
            border: 1px solid %4;
            border-radius: 6px;
            padding: 6px 14px;
        }
        QPushButton:hover {
            background-color: %8;
        }
        QPushButton:checked {
            background-color: %9;
            border-color: %9;
            color: white;
        }
        QStatusBar {
            background-color: %3;
            border-top: 1px solid %4;
            color: %7;
        }
        QSplitter::handle {
            background-color: %4;
        }
        QLabel[role="sectionTitle"] {
            font-weight: 700;
            color: %2;
        }
        QLabel[role="liveTitle"] {
            font-weight: 700;
            color: %10;
        }
        QLabel[role="muted"] {
            color: %7;
        }
    )")
        .arg(Colors::background)     // %1
        .arg(Colors::textPrimary)    // %2
        .arg(Colors::panel)          // %3
        .arg(Colors::border)         // %4
        .arg(Colors::panelAlt)       // %5
        .arg(Colors::accentSelect)   // %6
        .arg(Colors::textMuted)      // %7
        .arg("#33343c")              // %8 hover shade, close to panelAlt
        .arg(Colors::accentRecord)   // %9
        .arg(Colors::accentLive);    // %10

    app.setStyleSheet(qss);
}

} // namespace Theme
