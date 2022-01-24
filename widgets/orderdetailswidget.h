#pragma once

#include "structs.h"

#include <QSettings>
#include <QWidget>

namespace Ui { class OrderDetailsWidget; }

class OrderManager;
struct SharedData;

class OrderDetailsWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit OrderDetailsWidget(const bool standalone = false, QWidget *parent = nullptr);
        ~OrderDetailsWidget() override;

        void setSharedData(SharedData *shared);
        void setOrderManager(OrderManager *orderMgr);
        void setOrder(const Order &order);

    public slots:
        void updateOrderButtons(const int row, const int rowCount);
        void processOrderButton();
        void readSettings(const QSettings &set);
        void writeSettings(QSettings &set) const;

    signals:
        void orderRequested(const int delta);
        void hideRequested();

    protected:
        void closeEvent(QCloseEvent *event) override;

    private:
        QVariantList saveHeaderStates() const;
        void restoreHeaderStates(const QVariantList &states);

    private:
        Order m_order{};
        Ui::OrderDetailsWidget *m_ui{};
        SharedData *m_shared;
};
