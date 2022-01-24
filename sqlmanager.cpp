#include "structs.h"
#include "sqlmanager.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

QList<SqlManager::TableInfo> SqlManager::TableInformation =
{
    // table_versions must be first
    { "table_versions",        1, "(`table` TEXT NOT NULL, `version` INTEGER NOT NULL)"                                                                                 },
    { "order_properties",      1, "(`order_id` INTEGER NOT NULL UNIQUE, `note` TEXT, PRIMARY KEY(`order_id`))"                                                          },
    { "order_packaging",       1, "(`order_id` INTEGER NOT NULL UNIQUE, `packaging_id` INTEGER NOT NULL, PRIMARY KEY(`order_id`))"                                      },
    { "order_item_properties", 1, "(`order_id` INTEGER NOT NULL, `item_idx` INTEGER NOT NULL, `packaged` INTEGER, PRIMARY KEY(`order_id`,`item_idx`))"                  },
    { "packaging_types",       1, "(`id` INTEGER NOT NULL UNIQUE, `name` TEXT NOT NULL, `stock` INTEGER NOT NULL, `restock_url` TEXT, PRIMARY KEY(`id` AUTOINCREMENT))" },
};

SqlManager::SqlManager(const QString &dbPath, QObject *parent)
    : QObject{parent}
    , m_dbPath(dbPath)
{
    // Make sure the path exists
    const QFileInfo pathInfo(dbPath);
    pathInfo.dir().mkpath(".");
}

SqlManager::~SqlManager()
{

}

QPair<bool, QString> SqlManager::init()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(m_dbPath);
    if (!db.open())
        return qMakePair(false, db.lastError().text());

    return processTables();
}

int SqlManager::tableVersion(const QString &tableName)
{
    QSqlQuery query;
    query.prepare("SELECT version FROM table_versions WHERE `table` = :table;");
    query.bindValue(":table", tableName);
    if (!query.exec() || !query.next()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError();
        return -1;
    }

    return query.value(0).toInt();
}

QPair<bool, QString> SqlManager::setTableVersion(const QString &tableName, const int version)
{
    QSqlQuery query;
    query.prepare("INSERT INTO table_versions (`table`, `version`) VALUES (:table, :version);");
    query.bindValue(":table", tableName);
    query.bindValue(":version", version);

    if (!query.exec())
        return qMakePair(false, query.lastQuery() + " failed " + query.lastError().text());

    return qMakePair(true, QString{});
}

void SqlManager::restore(Order &order)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM order_packaging WHERE order_id = :order_id;");
    query.bindValue(":order_id", order.id);
    if (!query.exec()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return;
    }

    if (query.next()) {
        order.packaging = query.value("packaging_id").toInt();
    }


    query.prepare("SELECT * FROM order_item_properties WHERE order_id = :order_id;");
    query.bindValue(":order_id", order.id);
    if (!query.exec()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return;
    }

    while (query.next()) {
        const int itemIdx = query.value("item_idx").toInt();
        Item &item = order.items[itemIdx];

        item.packaged = query.value("packaged").toBool();
    }
}

void SqlManager::save(const Order &order)
{
    if (order.packaging > -1) {
        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO order_packaging (`order_id`, `packaging_id`) VALUES (:order_id, :packaging_id);");
        query.bindValue(":order_id", order.id);
        query.bindValue(":packaging_id", order.packaging);
        if (!query.exec()) {
            qDebug() << query.lastQuery() << "failed" << query.lastError().text();
            return;
        }
    }

    for (int i = 0; i < order.items.count(); ++i) {
        const Item &item = order.items[i];

        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO order_item_properties (`order_id`, `item_idx`, `packaged`) VALUES (:order_id, :item_idx, :packaged);");
        query.bindValue(":order_id", order.id);
        query.bindValue(":item_idx", i);
        query.bindValue(":packaged", item.packaged);
        if (!query.exec()) {
            qDebug() << query.lastQuery() << "failed" << query.lastError().text();
            return;
        }
    }
}

QList<Packaging> SqlManager::packagings() const
{
    QList<Packaging> result;

    QSqlQuery query;
    if (!query.exec("SELECT * FROM packaging_types ORDER BY id;")) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return result;
    }

    while (query.next())
        result << Packaging{ query.value("id").toInt(), query.value("name").toString(), query.value("stock").toInt(), query.value("restock_url").toString() };

    return result;
}

bool SqlManager::updatePackaging(const Packaging &pack)
{
    QSqlQuery query;
    if (pack.id == -1) {
        query.prepare("INSERT OR REPLACE INTO packaging_types (`name`, `stock`, `restock_url`) VALUES (:name, :stock, :restock_url);");
    } else {
        query.prepare("INSERT OR REPLACE INTO packaging_types (`id`, `name`, `stock`, `restock_url`) VALUES (:id, :name, :stock, :restock_url);");
        query.bindValue(":id", pack.id);
    }

    query.bindValue(":name", pack.name);
    query.bindValue(":stock", pack.stock);
    query.bindValue(":restock_url", pack.restockUrl);

    if (!query.exec()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return false;
    }

    return true;
}

bool SqlManager::removePackaging(const int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM packaging_types WHERE id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return false;
    }

    return true;
}

int SqlManager::ordersWithPackaging(const int id)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM order_packaging WHERE packaging_id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return 999; // beter safe than sorry
    }

    if (!query.next()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return 999; // beter safe than sorry
    }

    return query.value(0).toInt();
}

int SqlManager::packagingStock(const int id)
{
    QSqlQuery query;
    query.prepare("SELECT stock FROM packaging_types WHERE id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return 0;
    }

    if (!query.next()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return 0;
    }

    return query.value(0).toInt();
}

bool SqlManager::setPackagingStock(const int id, const int stock)
{
    QSqlQuery query;
    query.prepare("UPDATE packaging_types SET stock = :stock WHERE id = :id;");
    query.bindValue(":stock", stock);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << query.lastQuery() << "failed" << query.lastError().text();
        return false;
    }

    return true;
}

QPair<bool, QString> SqlManager::processTables()
{
    QSqlDatabase db = QSqlDatabase::database();

    for (const TableInfo &tableInfo : TableInformation) {
        const int tableVer = tableVersion(tableInfo.name);

        if (tableVer == -1) { // table doesn't exist
            const QSqlQuery query = db.exec(QString("CREATE TABLE %1 %2;").arg(tableInfo.name, tableInfo.sql));
            if (query.lastError().isValid())
                return qMakePair(false, query.lastError().text());

            const QPair<bool, QString> res = setTableVersion(tableInfo.name, tableInfo.version);
            if (!res.first)
                return res;
        } else if (tableVer > tableInfo.version) { // table newer than expected (woah)
            return qMakePair(false, QObject::tr("Your database seems to be newer than this version of Lectronizer supports.\n"
                                                "You have to either upgrade Lectronizer or delete the database file.\n"
                                                "Sorry for the inconvenience."));
        } else if(tableVer < tableInfo.version) { // table older than expected
            // TODO: upgrade
        }
    }

    return qMakePair(true, QString{});
}
