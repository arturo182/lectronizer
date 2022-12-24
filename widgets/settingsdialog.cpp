#include "settingsdialog.h"
#include "settingspages/generalsettingspage.h"
#include "settingspages/ordersettingspage.h"
#include "settingspages/packagingsettingspage.h"
#include "ui_settingsdialog.h"

#include <QSettings>
#include <QStyledItemDelegate>


class CategoryListDelegate : public QStyledItemDelegate
{
    public:
        explicit CategoryListDelegate(QObject *parent)
            : QStyledItemDelegate{parent}
        {

        }

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            QSize size = QStyledItemDelegate::sizeHint(option, index);
            size.setHeight(qMax(size.height(), 32));
            return size;
        }
};

SettingsDialog::SettingsDialog(OrderManager *orderMgr, SqlManager *sqlMgr, QWidget *parent)
    : QDialog(parent)
    , m_orderMgr{orderMgr}
    , m_sqlMgr{sqlMgr}
    , m_ui{new Ui::SettingsDialog}
{
    m_ui->setupUi(this);
    m_ui->categoryList->setItemDelegate(new CategoryListDelegate(this));

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [this]()
    {
        emit applied();
        accept();
    });

    connect(m_ui->categoryList, &QListWidget::currentRowChanged, this, [this](const int idx)
    {
        m_ui->stackedWidget->setCurrentIndex(idx);
    });

    addPage(new GeneralSettingsPage(this));
    addPage(new OrderSettingsPage(this));
    addPage(new PackagingSettingsPage(this));

    m_ui->categoryList->setCurrentRow(0);

    readSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete m_ui;
    m_ui = nullptr;
}

SharedData SettingsDialog::data()
{
    for (int i = 0; i < m_ui->stackedWidget->count(); ++i) {
        SettingPage *page = qobject_cast<SettingPage*>(m_ui->stackedWidget->widget(i));
        if (!page)
            continue;

        page->writeSettings(m_data);
    }

    return m_data;
}

void SettingsDialog::setData(const SharedData &data)
{
    m_data = data;

    for (int i = 0; i < m_ui->stackedWidget->count(); ++i) {
        SettingPage *page = qobject_cast<SettingPage*>(m_ui->stackedWidget->widget(i));
        if (!page)
            continue;

        page->readSettings(m_data);
    }
}

void SettingsDialog::addPage(SettingPage *page)
{
    QListWidgetItem *listItem = new QListWidgetItem(m_ui->categoryList);
    listItem->setText(page->pageName());
    listItem->setIcon(page->pageIcon());

    m_ui->stackedWidget->addWidget(page);

    page->init(m_orderMgr, m_sqlMgr);
}

void SettingsDialog::done(int r)
{
    writeSettings();

    QDialog::done(r);
}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    writeSettings();

    QWidget::closeEvent(event);
}

void SettingsDialog::readSettings()
{
    QSettings set;

    set.beginGroup("SettingsDialog");
    restoreGeometry(set.value("geometry").toByteArray());
    setWindowState(Qt::WindowStates(set.value("state").toInt()));
}

void SettingsDialog::writeSettings() const
{
    QSettings set;

    set.beginGroup("SettingsDialog");
    set.setValue("geometry", saveGeometry());
    set.setValue("state", (int)windowState());
}
