#include "mainwindow.h"
#include <QApplication>
#include "loginwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    LoginWindow loginWindow;

    if (loginWindow.exec() != QDialog::Accepted || !loginWindow.isAuthenticated()) {
        return 0;
    }

    MainWindow window;
    window.show();
    return app.exec();
}
