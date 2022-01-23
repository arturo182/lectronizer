#include "ordermanager.h"
#include "sqlmanager.h"
#include "widgets/mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName("Lectronizer");
    QApplication::setOrganizationName("arturo182");
    QApplication::setApplicationVersion(QString("%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_PATCH));
    QSettings::setDefaultFormat(QSettings::IniFormat);

    const QCommandLineOption apiUrlOption({ "u", "api-url" },
                                          QObject::tr("Use <api-url> to fetch orders.", "main"),
                                          QObject::tr("api-url", "main"));

    const QCommandLineOption dbPathOption({ "d", "db-file" },
                                          QObject::tr("Use <db-file> as the database.", "main"),
                                          QObject::tr("db-file", "main"),
                                          QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sqlite.db");

    QApplication app(argc, argv);

    // setup command line
    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("A client for the lectronz.com maker marketplace.", "main"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(apiUrlOption);
    parser.addOption(dbPathOption);
    parser.process(app);

    const QString apiUrl = parser.value(apiUrlOption);
    if (!apiUrl.isEmpty()) {
        OrderManager::ApiUrl = apiUrl;
        qDebug() << "Using Api URL:" << apiUrl;
    }

    // setup sql stuff
    SqlManager sqlMgr(parser.value(dbPathOption));
    const QPair<bool, QString> res = sqlMgr.init();
    if (!res.first) {
        QMessageBox::critical(nullptr, QObject::tr("Database Error"), res.second);

        return -1;
    }

    // show main window
    MainWindow mainWnd(&sqlMgr);
    mainWnd.show();

    return app.exec();
}
