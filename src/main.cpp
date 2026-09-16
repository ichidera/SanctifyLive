#include <QApplication>

#include "hardware/HardwareSettings.h"
#include "ui/OperatorWindow.h"
#include "ui/Theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SanctifyLive");
    QApplication::setOrganizationName("SanctifyLive");
    app.setStyleSheet(Theme::stylesheet());

    // Phase 1 foundation: detect (or, on every run after the first,
    // just recall) which hardware tier this machine falls into, and
    // persist it. Purely invisible right now -- nothing in the UI reads
    // this yet -- but it's what a future compositor, transition system,
    // or thumbnail pipeline will consult instead of assuming every
    // machine can handle motion backgrounds and live video compositing.
    // See src/hardware/ for the full detection -> scoring -> settings
    // pipeline and hardware/HardwareSettings.h for how to read it back.
    HardwareSettings::currentTier();

    OperatorWindow operatorWindow;
    operatorWindow.show();

    return QApplication::exec();
}
