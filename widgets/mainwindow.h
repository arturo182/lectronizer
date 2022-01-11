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
        void addColumnFilter(const int column, const QString &name, std::function<QString(const Order&)> getter);
        void fetchOrders(const int offset = 0, const int limit = 20);
        void fetchCurrencyRates();
        void updateDateFilter();
        void updateOrderDetails(const QItemSelection &selected);
        void readSettings();
        void writeSettings() const;
        void connectSignals();
        void processOrderButton();
        void showSettingsDialog();
        void showAboutDialog();
        void openOrderWindow(const int id);
        void orderTreeKeyPressEvent(QKeyEvent *event);
        int orderIdFromProxyModel(const QModelIndex &proxyIndex);

    private:
        QNetworkAccessManager *m_nam{};
        OrderSortFilterModel m_orderProxyModel{};
        QStandardItemModel m_orderModel{};
        QHash<int, Order> m_orders{};
        int m_orderOffset{0};
        SharedData m_shared{};
        int m_totalOrders{0};
        Ui::MainWindow *m_ui{};
};
