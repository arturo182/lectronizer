#pragma once

#include "shareddata.h"

#include <QDialog>

namespace Ui { class SettingsDialog; }

class OrderManager;
class SqlManager;

class SettingPage : public QWidget
{
    Q_OBJECT

    public:
        SettingPage(QWidget *parent) : QWidget(parent) { }
        ~SettingPage() override { }

        virtual void init(OrderManager *, SqlManager *) { }

        virtual QString pageName() const = 0;
        virtual QPixmap pageIcon() const = 0;

        virtual void readSettings(const SharedData &shared) = 0;
        virtual void writeSettings(SharedData &shared) = 0;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        SettingsDialog(OrderManager *orderMgr, SqlManager *sqlMgr, QWidget *parent = nullptr);
        ~SettingsDialog();

        SharedData data();
        void setData(const SharedData &data);

        void addPage(SettingPage *page);

    signals:
        void applied();

    protected:
        void closeEvent(QCloseEvent *event) override;

    private:
        void readSettings();
        void writeSettings() const;

    private:
        SharedData m_data{};
        OrderManager *m_orderMgr{};
        SqlManager *m_sqlMgr{};
        Ui::SettingsDialog *m_ui{};
};

