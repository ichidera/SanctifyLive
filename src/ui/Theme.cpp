#include "ui/Theme.h"

namespace Theme
{

QString stylesheet()
{
    return QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            font-family: "Segoe UI", "Inter", "Helvetica Neue", sans-serif;
            font-size: 13px;
        }

        QMainWindow, QDialog {
            background-color: %1;
        }

        /* ---- Toolbar (New/Open/Save/.../Go Live/Alerts/.../Live) ---- */
        QToolBar {
            background-color: %3;
            border: none;
            border-bottom: 1px solid %4;
            padding: 6px 10px;
            spacing: 6px;
        }
        QToolBar QToolButton {
            background: transparent;
            color: %2;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QToolBar QToolButton:hover {
            background-color: %5;
            border: 1px solid %4;
        }
        QToolBar QToolButton:checked {
            background-color: %5;
            border: 1px solid %6;
        }
        QToolBar::separator {
            background-color: %4;
            width: 1px;
            margin: 4px 6px;
        }

        /* Primary call-to-action (Go Live) */
        QPushButton#goLiveButton {
            background-color: %7;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 7px 18px;
            font-weight: 600;
        }
        QPushButton#goLiveButton:hover { background-color: #ea5b45; }
        QPushButton#goLiveButton:checked { background-color: #a8321f; }

        /* Black/Clear toolbar buttons: quiet until active, then red */
        QToolButton#blackButton:checked, QToolButton#clearButton:checked {
            background-color: %7;
            border: 1px solid %7;
            color: white;
        }

        /* ---- Generic buttons elsewhere in the app ---- */
        QPushButton {
            background-color: %5;
            color: %2;
            border: 1px solid %4;
            border-radius: 6px;
            padding: 6px 12px;
        }
        QPushButton:hover { background-color: %8; border-color: %6; }
        QPushButton:pressed { background-color: %4; }
        QPushButton:disabled { color: %9; }
        QPushButton:checked {
            background-color: %7;
            border-color: %7;
            color: white;
        }

        /* ---- Panels: Schedule / Preview / Live / Media / History cards ---- */
        QFrame#panelCard {
            background-color: %3;
            border: 1px solid %4;
            border-radius: 8px;
        }
        QLabel#panelTitle {
            color: %9;
            font-weight: 600;
            font-size: 11px;
            letter-spacing: 1px;
            text-transform: uppercase;
            padding: 2px 4px;
        }
        QLabel#liveTitle {
            color: %7;
            font-weight: 700;
            font-size: 11px;
            letter-spacing: 1px;
            padding: 2px 4px;
        }
        QLabel#liveDot {
            color: %7;
            font-weight: 700;
        }
        QLabel#nextSlideLabel {
            color: %9;
            font-style: italic;
            padding: 4px 2px;
        }

        /* ---- Bottom content tabs: Songs/Scriptures/Media/... ---- */
        QTabWidget::pane {
            border: 1px solid %4;
            border-radius: 8px;
            background-color: %3;
            top: -1px;
        }
        QTabBar::tab {
            background: transparent;
            color: %9;
            padding: 8px 16px;
            margin-right: 2px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QTabBar::tab:hover { color: %2; }
        QTabBar::tab:selected {
            color: %2;
            background-color: %3;
            border: 1px solid %4;
            border-bottom: none;
        }

        /* ---- Lists / Trees (Schedule, History, Media folder tree) ---- */
        QListWidget, QTreeWidget {
            background-color: %3;
            border: 1px solid %4;
            border-radius: 6px;
            outline: none;
        }
        QListWidget::item {
            padding: 8px 8px;
            border-bottom: 1px solid %4;
            color: %2;
        }
        QListWidget::item:selected {
            background-color: rgba(59, 130, 246, 0.18);
            border-left: 3px solid %6;
            color: %2;
        }
        QListWidget::item:hover:!selected { background-color: %5; }

        QTreeWidget::item { padding: 5px 4px; }
        QTreeWidget::item:selected {
            background-color: rgba(59, 130, 246, 0.18);
            color: %2;
        }

        /* ---- Media thumbnail tiles ---- */
        QFrame#mediaTile {
            background-color: %5;
            border: 2px solid transparent;
            border-radius: 8px;
        }
        QFrame#mediaTile:hover { border: 2px solid %4; }
        QFrame#mediaTile[selected="true"] { border: 2px solid %6; }
        QLabel#mediaTileLabel {
            color: %2;
            font-size: 11px;
        }

        /* ---- Status bar ---- */
        QStatusBar {
            background-color: %3;
            border-top: 1px solid %4;
            color: %9;
        }
        QStatusBar::item { border: none; }
        QLabel#stageStatusDot { color: %10; font-weight: 700; }

        /* ---- Misc ---- */
        QSplitter::handle { background-color: %1; }
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: %4;
            border-radius: 5px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: %6; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

        QGroupBox {
            border: 1px solid %4;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 10px;
            font-weight: 600;
            color: %9;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }

        QToolTip {
            background-color: %3;
            color: %2;
            border: 1px solid %4;
            padding: 4px 6px;
        }
    )")
        .arg(kBackground)     // %1
        .arg(kTextPrimary)    // %2
        .arg(kPanel)          // %3
        .arg(kBorder)         // %4
        .arg(kPanelAlt)       // %5
        .arg(kAccentBlue)     // %6
        .arg(kAccentRed)      // %7
        .arg("#262a34")       // %8 hover shade
        .arg(kTextMuted)      // %9
        .arg(kAccentGreen);   // %10
}

} // namespace Theme
