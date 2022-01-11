#pragma once

#include <QDialog>

namespace Ui { class SettingsDialog; }

class SettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit SettingsDialog(QWidget *parent = nullptr);
        ~SettingsDialog();

        void setCurrencies(QStringList currencies);

        QString apiKey() const;
        void setApiKey(const QString &apiKey);

        QString targetCurrency() const;
        void setTargetCurrency(const QString &targetCurrency);

    private:
        Ui::SettingsDialog *m_ui{};
};

