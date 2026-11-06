#include "MainWindow.hpp"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("ESP32 Lab Studio");
    QApplication::setOrganizationName("ESP32LabStudio");

    MainWindow window;
    window.show();
    return app.exec();
}
