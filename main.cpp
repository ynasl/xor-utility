#include "mainwindow.h"
#include "types.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    qRegisterMetaType<AppConfig>("AppConfig");

    return a.exec();
}
