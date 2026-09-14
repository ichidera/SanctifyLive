#include <QApplication>

#include "ui/OperatorWindow.h"
#include "ui/Theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SanctifyLive");
    QApplication::setOrganizationName("SanctifyLive");
    app.setStyleSheet(Theme::stylesheet());

    OperatorWindow operatorWindow;
    operatorWindow.show();

    return QApplication::exec();
}
