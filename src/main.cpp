#include <QApplication>
#include <QPalette>

#include "core/OperatorWindow.h"

namespace {
// A dark operator theme, in the spirit of EasyWorship/ProPresenter's
// dark interface mode -- easier on the eyes in a dim sanctuary, and it
// makes the LIVE/PREVIEW panes and accent colors read clearly.
void applyDarkTheme(QApplication &app)
{
    app.setStyle("Fusion");

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#202124"));
    palette.setColor(QPalette::WindowText, QColor("#e8e8e8"));
    palette.setColor(QPalette::Base, QColor("#151517"));
    palette.setColor(QPalette::AlternateBase, QColor("#1c1c1f"));
    palette.setColor(QPalette::Text, QColor("#e8e8e8"));
    palette.setColor(QPalette::Button, QColor("#2b2b2e"));
    palette.setColor(QPalette::ButtonText, QColor("#e8e8e8"));
    palette.setColor(QPalette::Highlight, QColor("#2d8cf0"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::PlaceholderText, QColor("#888888"));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#5a5a5a"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#5a5a5a"));
    app.setPalette(palette);

    app.setStyleSheet(R"(
        QMainWindow, QWidget { background-color: #202124; }
        QTabWidget::pane { border: 1px solid #333; background-color: #1c1c1f; }
        QTabBar::tab {
            background-color: #2b2b2e; color: #cfcfcf;
            padding: 6px 8px; border: 1px solid #333; border-bottom: none;
        }
        QTabBar::tab:selected { background-color: #1c1c1f; color: white; }
        QTabBar::tab:disabled { color: #555; }
        QListWidget {
            background-color: #151517; border: 1px solid #333;
            alternate-background-color: #1a1a1d;
        }
        QListWidget::item { padding: 6px; }
        QListWidget::item:selected { background-color: #2d8cf0; color: white; }
        QPushButton {
            background-color: #33343a; color: #e8e8e8;
            border: 1px solid #444; border-radius: 4px; padding: 8px 14px;
        }
        QPushButton:hover { background-color: #3d3e45; }
        QPushButton:pressed { background-color: #2a2b30; }
        QPushButton#goLiveButton {
            background-color: #27ae60; color: white; font-weight: 700; font-size: 14px;
        }
        QPushButton#goLiveButton:hover { background-color: #2ecc71; }
        QPushButton#blackButton:checked {
            background-color: #c0392b; color: white; border: 1px solid #922b21;
        }
        QWidget#transportBar {
            background-color: #1a1a1d; border-top: 1px solid #333;
            padding: 6px; border-radius: 4px;
        }
        QSplitter::handle { background-color: #2b2b2e; }
    )");
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SanctifyLive");
    QApplication::setOrganizationName("SanctifyLive");

    applyDarkTheme(app);

    OperatorWindow operatorWindow;
    operatorWindow.show();

    return QApplication::exec();
}