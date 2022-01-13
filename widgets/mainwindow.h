#pragma once

#include "ordersortfiltermodel.h"
#include "shareddata.h"
#include "structs.h"

#include <QItemSelection>
#include <QMainWindow>
#include <QStandardItemModel>

namespace Ui { class MainWindow; }

class QCloseEvent;
class QNetworkAccessManager;

class OrderManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override;

    protected:
        void closeEvent(QCloseEvent *event) override;
        bool eventFilter(QObject *object, QEvent *event) override;

    private:        
        void readSettings();
        void writeSettings() const;

        void connectSignals();
        void openOrderWindow(const int id);
        void orderTreeKeyPressEvent(QKeyEvent *event);
        int orderIdFromProxyModel(const QModelIndex &proxyIndex);
        void syncOrderRow(const int row, const Order &order);

        void fetchCurrencyRates();
        double convertCurrency(const double eur);
        QString convertCurrencyString(const double eur);

    private slots:
        void addOrder(const Order &order);
        void updateOrder(const Order &order);
        void updateDateFilter();
        void updateOrderDetails(const QItemSelection &selected);
        void updateTreeStatsLabel();
        void showSettingsDialog();
        void showAboutDialog();

    private:
        QNetworkAccessManager *m_nam{};
        OrderSortFilterModel m_orderProxyModel{};
        QStandardItemModel m_orderModel{};
        OrderManager *m_orderMgr{};
        SharedData m_shared{};
        Ui::MainWindow *m_ui{};
};
