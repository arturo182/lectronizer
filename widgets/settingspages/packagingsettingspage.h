#pragma once

#include "widgets/settingsdialog.h"

namespace Ui { class PackagingSettingsPage; }

class PackagingSettingsPage : public SettingPage
{
    Q_OBJECT

    public:
        explicit PackagingSettingsPage(QWidget *parent = nullptr);
        ~PackagingSettingsPage() override;

        void init(OrderManager *orderMgr, SqlManager *sqlMgr) override;

        QString pageName() const override;
        QPixmap pageIcon() const override;

        void readSettings(const SharedData &shared) override;
        void writeSettings(SharedData &shared) override;

    private:
        SqlManager *m_sqlMgr{};
        QList<int> m_removedIds{};
        Ui::PackagingSettingsPage *m_ui;
};

