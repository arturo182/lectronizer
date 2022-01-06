#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QToolTip>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::SettingsDialog)
{
    m_ui->setupUi(this);

    const auto showToolTip = [&]()
    {
        QPushButton *button = qobject_cast<QPushButton*>(sender());

        QToolTip::showText(button->mapToGlobal(QPoint()), button->toolTip());
    };

    connect(m_ui->apiKeyHelpButton, &QPushButton::pressed, this, showToolTip);
    connect(m_ui->targetCurrencyHelpButton, &QPushButton::pressed, this, showToolTip);
}

SettingsDialog::~SettingsDialog()
{
    delete m_ui;
}

void SettingsDialog::setCurrencies(QStringList currencies)
{
    std::sort(currencies.begin(), currencies.end());

    m_ui->targetCurrencyCombo->addItems(currencies);
}

QString SettingsDialog::apiKey() const
{
    return m_ui->apiKeyEdit->text();
}

void SettingsDialog::setApiKey(const QString &apiKey)
{
    m_ui->apiKeyEdit->setText(apiKey);
}

QString SettingsDialog::targetCurrency() const
{
    return m_ui->targetCurrencyCombo->currentText();
}

void SettingsDialog::setTargetCurrency(const QString &targetCurrency)
{
    m_ui->targetCurrencyCombo->setCurrentText(targetCurrency);
}
