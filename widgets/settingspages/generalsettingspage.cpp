#include "generalsettingspage.h"
#include "ui_generalsettingspage.h"

#include <QDesktopServices>
#include <QToolTip>
#include <QUrl>

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
    connect(m_ui->dateFormatHelpButton, &QPushButton::pressed, []()
    {
        QDesktopServices::openUrl(QUrl("https://doc.qt.io/qt-6/qdatetime.html#toString"));
    });
    connect(m_ui->shippingTrackingUrlHelpButton, &QPushButton::pressed, this, showToolTip);

    connect(m_ui->autoFetchCheckBox, &QCheckBox::stateChanged, m_ui->autoFetchIntervalSpinBox, &QSpinBox::setEnabled);
}

GeneralSettingsPage::~GeneralSettingsPage()
{
    delete m_ui;
    m_ui = nullptr;
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
    m_ui->closeToTrayCheckBox->setChecked(shared.closeToSystemTray);
    m_ui->autoFetchCheckBox->setChecked(shared.autoFetchWhenMinimized);
    m_ui->dateFormatEdit->setText(shared.dateFormat);
    m_ui->friendlyDateCheckBox->setChecked(shared.friendlyDate);
    m_ui->autoFetchIntervalSpinBox->setValue(shared.autoFetchIntervalMin);
    m_ui->shippingTrackingUrlEdit->setText(shared.trackingUrl);
    m_ui->csvSeparatorComboBox->setCurrentIndex(shared.csvSeparator);
}

void GeneralSettingsPage::writeSettings(SharedData &shared)
{
    shared.apiKey = m_ui->apiKeyEdit->text();
    shared.targetCurrency = m_ui->targetCurrencyCombo->currentText();
    shared.closeToSystemTray = m_ui->closeToTrayCheckBox->isChecked();
    shared.autoFetchWhenMinimized = m_ui->autoFetchCheckBox->isChecked();
    shared.dateFormat = m_ui->dateFormatEdit->text();
    shared.friendlyDate = m_ui->friendlyDateCheckBox->isChecked();
    shared.autoFetchIntervalMin = m_ui->autoFetchIntervalSpinBox->value();
    shared.trackingUrl = m_ui->shippingTrackingUrlEdit->text();
    shared.csvSeparator = m_ui->csvSeparatorComboBox->currentIndex();
}
