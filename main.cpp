#include "ordermanager.h"
#include "widgets/mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Lectronizer");
    QApplication::setOrganizationName("arturo182");
    QApplication::setApplicationVersion(QString("%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH));
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("main", "A client for the lectronz.com maker marketplace."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption apiUrlOption({ "u", "api-url" }, QObject::tr("main", "Use <api-url> to fetch orders."), QObject::tr("main", "api-url"));
    parser.addOption(apiUrlOption);

    QApplication app(argc, argv);

    parser.process(app);

    const QString apiUrl = parser.value(apiUrlOption);
    if (!apiUrl.isEmpty()) {
        OrderManager::ApiUrl = apiUrl;
        qDebug() << "Using Api URL:" << apiUrl;
    }

    MainWindow mainWnd;
    mainWnd.show();

    return app.exec();
}
