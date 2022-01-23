#pragma once

#include "widgets/settingsdialog.h"

namespace Ui { class GeneralSettingsPage; }

class GeneralSettingsPage : public SettingPage
{
    Q_OBJECT

    public:
        explicit GeneralSettingsPage(QWidget *parent = nullptr);
        ~GeneralSettingsPage() override;

        QString pageName() const override;
        QPixmap pageIcon() const override;

        void readSettings(const SharedData &shared) override;
        void writeSettings(SharedData &shared) override;

    private:
        Ui::GeneralSettingsPage *m_ui;
};

