#include "widgets/mainwindow.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Lectronizer");
    QApplication::setOrganizationName("arturo182");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QApplication app(argc, argv);

    MainWindow mainWnd;
    mainWnd.show();

    return app.exec();
}
