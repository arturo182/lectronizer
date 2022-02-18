#pragma once

#include "structs.h"

#include <QHash>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;

struct SharedData;
class SqlManager;

class OrderManager : public QObject
{
    Q_OBJECT

    public:
        static QString ApiUrl;

    public:
        OrderManager(QNetworkAccessManager *nam, SharedData *shared, SqlManager *sqlMgr, QWidget *parent = nullptr);
        ~OrderManager() override;

        void refresh(const bool hidden);

        bool contains(const int id) const;

        Order &order(const int id);

        QList<int> orderIds() const;

        void markShipped(const int id, const QString &trackingNo = QString(), const QString &trackingUrl = QString());

    private:
        void resetProgressDlg();
        void fetch(const int offset, const int limit);
        void processFetch(const QJsonObject &root);
        void setErrorMsg(const QString &error);

    signals:
        void orderReceived(const Order &order);
        void orderUpdated(const Order &order);
        void refreshCompleted(const int newOrder, const int updatedOrders);
        void refreshFailed(const QString &error);

    private:
        QNetworkAccessManager *m_nam{};
        int m_newOrders{};
        QHash<int, Order> m_orders{};
        QProgressDialog *m_progressDlg{};
        QNetworkReply *m_reply{};
        SharedData *m_shared{};
        SqlManager *m_sqlMgr{};
        int m_updatedOrders{};
};
