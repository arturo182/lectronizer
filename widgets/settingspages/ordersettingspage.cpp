#include "ordersettingspage.h"
#include "ui_ordersettingspage.h"

OrderSettingsPage::OrderSettingsPage(QWidget *parent)
    : SettingPage{parent}
    , m_ui{new Ui::OrderSettingsPage}
{
    m_ui->setupUi(this);
}

OrderSettingsPage::~OrderSettingsPage()
{
    delete m_ui;
    m_ui = nullptr;
}

QString OrderSettingsPage::pageName() const
{
    return tr("Orders");
}

QPixmap OrderSettingsPage::pageIcon() const
{
    return QPixmap(":/res/icons/box.png");
}

void OrderSettingsPage::readSettings(const SharedData &shared)
{
    m_ui->phoneRemoveDashesCheckBox->setChecked(shared.phoneRemoveDashes);
    m_ui->phoneRemoveSpacesCheckBox->setChecked(shared.phoneRemoveSpaces);
    m_ui->phoneAddCodeCheckBox->setChecked(shared.phoneAddCountryCode);
    m_ui->phonePrefixComboBox->setCurrentIndex(!shared.phoneUsePlusPrefix);

    m_ui->groupOrderDetailWindowsCheckBox->setChecked(shared.groupOrderDetailWindows);
}

void OrderSettingsPage::writeSettings(SharedData &shared)
{
    shared.phoneRemoveDashes = m_ui->phoneRemoveDashesCheckBox->isChecked();
    shared.phoneRemoveSpaces = m_ui->phoneRemoveSpacesCheckBox->isChecked();
    shared.phoneAddCountryCode = m_ui->phoneAddCodeCheckBox->isChecked();
    shared.phoneUsePlusPrefix = (m_ui->phonePrefixComboBox->currentIndex() == 0);

    shared.groupOrderDetailWindows = m_ui->groupOrderDetailWindowsCheckBox->isChecked();
}
