#pragma once
//
// Theme.h
// Central place for the dark color palette + shared stylesheet fragments so
// every panel looks consistent. Pure presentation only - no behavior.
//
#include <QString>

namespace Theme {

inline constexpr const char* kBgApp        = "#1b1d21";
inline constexpr const char* kBgPanel      = "#202226";
inline constexpr const char* kBgPanelAlt   = "#26282d";
inline constexpr const char* kBgInset      = "#14161a";
inline constexpr const char* kBorder       = "#33363c";
inline constexpr const char* kAccentRed    = "#e6432f";
inline constexpr const char* kAccentGreen  = "#3ecf6a";
inline constexpr const char* kAccentBlue   = "#3a8dde";
inline constexpr const char* kTextPrimary  = "#f2f2f2";
inline constexpr const char* kTextSecondary= "#9aa0a8";

// Applied once to the whole application (qApp->setStyleSheet).
inline QString globalStyleSheet()
{
    return QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            font-family: "Segoe UI", "Noto Sans", sans-serif;
            font-size: 12px;
        }
        QMainWindow, QStatusBar {
            background-color: %1;
        }
        QGroupBox {
            background-color: %3;
            border: 1px solid %4;
            border-radius: 6px;
            margin-top: 14px;
            font-weight: 600;
            padding-top: 6px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: %2;
        }
        QSplitter::handle {
            background-color: %4;
        }
        QSplitter::handle:horizontal { width: 3px; }
        QSplitter::handle:vertical { height: 3px; }
        QSplitter::handle:hover { background-color: %5; }

        QTreeWidget, QListWidget, QTextEdit, QListView {
            background-color: %6;
            border: 1px solid %4;
            border-radius: 4px;
        }
        QTreeWidget::item, QListWidget::item {
            padding: 4px 2px;
        }
        QTreeWidget::item:selected, QListWidget::item:selected {
            background-color: %5;
            color: white;
        }

        QTabWidget::pane {
            border: 1px solid %4;
            top: -1px;
        }
        QTabBar::tab {
            background: %1;
            color: %7;
            padding: 6px 16px;
            border: 1px solid %4;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background: %3;
            color: %2;
            font-weight: 600;
        }

        QPushButton {
            background-color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 5px 12px;
        }
        QPushButton:hover { background-color: %5; }
        QPushButton:pressed { background-color: %4; }

        QToolButton {
            background-color: transparent;
            border: none;
            padding: 4px 10px;
            color: %2;
        }
        QToolButton:hover {
            background-color: %3;
            border-radius: 4px;
        }

        QLabel[role="brand"] {
            font-size: 15px;
            font-weight: 700;
        }
        QLabel[role="sectionTitle"] {
            font-size: 14px;
            font-weight: 700;
        }
        QLabel[role="liveLabel"] {
            color: %8;
            font-weight: 700;
            font-size: 13px;
        }
        QLabel[role="dim"] {
            color: %7;
        }

        QPushButton[role="goLive"] {
            background-color: %8;
            color: white;
            font-weight: 700;
            border: none;
        }
        QPushButton[role="goLive"]:hover { background-color: #ff5a44; }

        QScrollBar:vertical {
            background: transparent;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: %4;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
    )")
    .arg(kBgApp)        // %1
    .arg(kTextPrimary)  // %2
    .arg(kBgPanelAlt)   // %3
    .arg(kBorder)       // %4
    .arg(kAccentBlue)   // %5
    .arg(kBgInset)      // %6
    .arg(kTextSecondary)// %7
    .arg(kAccentRed);   // %8
}

} // namespace Theme
