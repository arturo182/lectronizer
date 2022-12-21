#pragma once

#include "widgets/settingsdialog.h"

namespace Ui { class OrderSettingsPage; }

class OrderSettingsPage : public SettingPage
{
    Q_OBJECT

    public:
        explicit OrderSettingsPage(QWidget *parent = nullptr);
        ~OrderSettingsPage() override;

        QString pageName() const override;
        QPixmap pageIcon() const override;

        void readSettings(const SharedData &shared) override;
        void writeSettings(SharedData &shared) override;

    private:
        Ui::OrderSettingsPage *m_ui;
};

