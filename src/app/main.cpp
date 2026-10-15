#include "ui/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("SignalForge");
    QCoreApplication::setApplicationName("SignalForge");
    QCoreApplication::setApplicationVersion("0.1.0");

    signalforge::ui::MainWindow window;
    window.show();
    return app.exec();
}
