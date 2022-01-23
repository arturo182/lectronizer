#pragma once

#include <QObject>

class Order;
class Packaging;

class SqlManager : public QObject
{
    Q_OBJECT

    public:
        struct TableInfo
        {
            QString name{};
            int version{};
            QString sql{};
        };

        static QList<TableInfo> TableInformation;

    public:
        explicit SqlManager(const QString &dbPath, QObject *parent = nullptr);
        ~SqlManager() override;

        QPair<bool, QString> init();

        int tableVersion(const QString &tableName);
        QPair<bool, QString> setTableVersion(const QString &tableName, const int version);

        void restore(Order &order);
        void save(const Order &order);

        QList<Packaging> packagings() const;
        bool updatePackaging(const Packaging &pack);
        bool removePackaging(const int id);

        int ordersWithPackaging(const int id);

        int packagingStock(const int id);
        bool setPackagingStock(const int id, const int stock);

    private:
        QPair<bool, QString> processTables();

    private:
        QString m_dbPath{};
};

