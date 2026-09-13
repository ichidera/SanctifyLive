#include <QApplication>

#include "core/ui/OperatorWindow.h"
#include "core/ui/Theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SanctifyLive");
    QApplication::setOrganizationName("SanctifyLive");

    Theme::applyDark(app);

    OperatorWindow operatorWindow;
    operatorWindow.show();

    return QApplication::exec();
}