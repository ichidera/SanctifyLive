#include <QApplication>

#include "core/OperatorWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SanctifyLive");
    QApplication::setOrganizationName("SanctifyLive");

    OperatorWindow operatorWindow;
    operatorWindow.show();

    return QApplication::exec();
}