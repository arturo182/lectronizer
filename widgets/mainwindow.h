#pragma once

#include "ordersortfiltermodel.h"
#include "shareddata.h"
#include "structs.h"

#include <QItemSelection>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QTimer>

namespace Ui { class MainWindow; }

class QCloseEvent;
class QNetworkAccessManager;
class QSystemTrayIcon;

class OrderManager;
class SqlManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        explicit MainWindow(SqlManager *sqlMgr, QWidget *parent = nullptr);
        ~MainWindow() override;

    protected:
        void closeEvent(QCloseEvent *event) override;
        bool eventFilter(QObject *obj, QEvent *event) override;
        bool event(QEvent *event) override;

    private:
        void readSettings();
        void writeSettings() const;

        void setupTrayIcon();
        void connectSignals();
        void openOrderWindow(const int id);

        int orderIdFromProxyModel(const QModelIndex &proxyIndex) const;
        int currentOrderId() const;

        void syncOrderRow(const int row, const Order &order);
        void syncAllOrderRows();

        void fetchCurrencyRates();
        double convertCurrency(const double eur);
        QString convertCurrencyString(const double eur);

    private slots:
        void addOrder(const Order &order);
        void updateOrder(const Order &order);
        void updateDateFilter();
        void updateOrderDetails(const QItemSelection &selected);
        void updateOrderRelatedWidgets();
        void updateTreeStatsLabel();
        void updateAutoFetchTimer();
        void showSettingsDialog();
        void showAboutDialog();

    private:
        QTimer m_autoFetchTimer{};
        QTimer m_dailySyncTimer{};
        bool m_firstFetch{};
        QNetworkAccessManager *m_nam{};
        OrderSortFilterModel m_orderProxyModel{};
        QStandardItemModel m_orderModel{};
        OrderManager *m_orderMgr{};
        bool m_reallyExit{false};
        SharedData m_shared{};
        SqlManager *m_sqlMgr{};
        QSystemTrayIcon *m_tray{};
        QMenu *m_trayMenu{};
        Ui::MainWindow *m_ui{};
};
