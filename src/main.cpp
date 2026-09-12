//
// main.cpp
// Entry point for the SanctifyLive UI scaffold. This wires up the app,
// applies the dark theme stylesheet, and shows MainWindow. No live/streaming
// behavior is implemented - this is interface scaffolding only.
//
#include <QApplication>
#include "MainWindow.h"
#include "Theme.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet(Theme::globalStyleSheet());

    MainWindow window;
    window.show();

    return app.exec();
}
