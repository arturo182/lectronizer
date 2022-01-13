#pragma once

#include "structs.h"

#include <QHash>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
class QProgressDialog;

class SharedData;

class OrderManager : public QObject
{
    Q_OBJECT

    public:
        OrderManager(QNetworkAccessManager *nam, SharedData *shared, QWidget *parent = nullptr);
        ~OrderManager() override;

        void refresh();

        bool contains(const int id) const;

        const Order &order(const int id);

    private:
        void resetProgressDlg();
        void fetch(const int offset, const int limit);
        void processFetch(const QJsonObject &root);
        void setErrorMsg(const QString &error);

    signals:
        void orderReceived(const Order &order);
        void orderUpdated(const Order &order);
        void refreshCompleted(const int newOrder, const int updatedOrders);

    private:
        QNetworkAccessManager *m_nam{};
        int m_newOrders{};
        QHash<int, Order> m_orders{};
        QProgressDialog *m_progressDlg{};
        QNetworkReply *m_reply{};
        SharedData *m_shared{};
        int m_updatedOrders{};
};
