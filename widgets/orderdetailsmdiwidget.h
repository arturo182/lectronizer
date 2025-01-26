#pragma once

#include <QWidget>

namespace Ui { class OrderDetailsMdiWidget; }

class OrderDetailsWidget;

class OrderDetailsMdiWidget : public QWidget
{
    Q_OBJECT

    static OrderDetailsMdiWidget *instance;

    public:
        static OrderDetailsMdiWidget *getInstance();

    public:
        explicit OrderDetailsMdiWidget(QWidget *parent = nullptr);
        ~OrderDetailsMdiWidget() override;

        void addWidget(OrderDetailsWidget *widget);

        bool hasOrder(const int id) const;

        int currentOrder() const;
        void setCurrentOrder(const int id);

    protected:
        void closeEvent(QCloseEvent *event) override;

    private slots:
        void updateWindowTitle();

    private:
        void readSettings();
        void writeSettings() const;

    private:
        Ui::OrderDetailsMdiWidget *m_ui{};
};
