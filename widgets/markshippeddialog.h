#pragma once

#include <QDialog>

namespace Ui { class MarkShippedDialog; }

class Order;
struct SharedData;

class MarkShippedDialog : public QDialog
{
    Q_OBJECT

    public:
        MarkShippedDialog(const Order &order, SharedData *sharedData, QWidget *parent = nullptr);
        ~MarkShippedDialog();

        QString trackingNo() const;
        QString trackingUrl() const;

    private:
        Ui::MarkShippedDialog *m_ui{};
        SharedData *m_sharedData{};
};
