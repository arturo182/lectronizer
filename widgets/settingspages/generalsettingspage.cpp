#include "generalsettingspage.h"
#include "ui_generalsettingspage.h"

#include <QToolTip>

GeneralSettingsPage::GeneralSettingsPage(QWidget *parent)
    : SettingPage{parent}
    , m_ui{new Ui::GeneralSettingsPage}
{
    m_ui->setupUi(this);

    const auto showToolTip = [&]()
    {
        QPushButton *button = qobject_cast<QPushButton*>(sender());

        QToolTip::showText(button->mapToGlobal(QPoint()), button->toolTip(), button, {}, 1000);
    };

    connect(m_ui->apiKeyHelpButton, &QPushButton::pressed, this, showToolTip);
    connect(m_ui->targetCurrencyHelpButton, &QPushButton::pressed, this, showToolTip);
}

GeneralSettingsPage::~GeneralSettingsPage()
{
    delete m_ui;
}

QString GeneralSettingsPage::pageName() const
{
    return tr("General");
}

QPixmap GeneralSettingsPage::pageIcon() const
{
    return QPixmap(":/res/icons/cog.png");
}

void GeneralSettingsPage::readSettings(const SharedData &shared)
{
    QStringList currencies = shared.currencyRates.keys();
    std::sort(currencies.begin(), currencies.end());

    m_ui->targetCurrencyCombo->addItems(currencies);
    m_ui->apiKeyEdit->setText(shared.apiKey);
    m_ui->targetCurrencyCombo->setCurrentText(shared.targetCurrency);
}

void GeneralSettingsPage::writeSettings(SharedData &shared)
{
    shared.apiKey = m_ui->apiKeyEdit->text();
    shared.targetCurrency = m_ui->targetCurrencyCombo->currentText();
}
