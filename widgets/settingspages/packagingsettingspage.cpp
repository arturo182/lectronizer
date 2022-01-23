#include "packagingsettingspage.h"
#include "structs.h"
#include "sqlmanager.h"
#include "ui_packagingsettingspage.h"

#include <QMessageBox>

enum
{
    IdRole     = Qt::UserRole + 0,
    UrlRole    = Qt::UserRole + 1,
    OrdersRole = Qt::UserRole + 2,
};

PackagingSettingsPage::PackagingSettingsPage(QWidget *parent)
    : SettingPage{parent}
    , m_ui{new Ui::PackagingSettingsPage}
{
    m_ui->setupUi(this);

    m_ui->treeWidget->header()->resizeSection(0, 250);
    m_ui->restockUrlEdit->setReadOnly(false);
    m_ui->detailsWidget->hide();
    m_ui->removeButton->setEnabled(false);

    QTreeWidgetItem *defaultItem = new QTreeWidgetItem(m_ui->treeWidget);
    defaultItem->setText(0, tr("Default packaging"));
    defaultItem->setData(0, IdRole, 0);
    defaultItem->setData(0, OrdersRole, 999);
    defaultItem->setText(1, "-");

    // update details widgets
    connect(m_ui->treeWidget, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item)
    {
        if (!item)
            return;

        m_ui->detailsWidget->show();

        m_ui->nameEdit->setText(item->text(0));
        m_ui->stockSpinBox->setValue(item->text(1).toInt());
        m_ui->restockUrlEdit->setText(item->data(0, UrlRole).toString());

        // Default packaging is read-only
        m_ui->detailsWidget->setEnabled(item != m_ui->treeWidget->topLevelItem(0));
        m_ui->removeButton->setEnabled(item != m_ui->treeWidget->topLevelItem(0));
    });

    // update name and mark modified
    connect(m_ui->nameEdit, &QLineEdit::textChanged, this, [this](const QString &text)
    {
        QTreeWidgetItem *item = m_ui->treeWidget->currentItem();
        if (!item)
            return;

        if (item->text(0) == text)
            return;

        if (text.isEmpty()) {
            m_ui->nameEdit->setText(tr("Unnamed"));
            return;
        }

        QFont italicFont = item->font(0);
        italicFont.setItalic(true);

        item->setText(0, text);
        item->setFont(0, italicFont);
    });

    // update url and mark modified
    connect(m_ui->restockUrlEdit, &ClickyLineEdit::textChanged, this, [this](const QString &text)
    {
        QTreeWidgetItem *item = m_ui->treeWidget->currentItem();
        if (!item)
            return;

        if (item->data(0, UrlRole).toString() == text)
            return;

        QFont italicFont = item->font(0);
        italicFont.setItalic(true);

        item->setData(0, UrlRole, text);
        item->setFont(0, italicFont);
    });

    // update stock and mark modified
    connect(m_ui->stockSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value)
    {
        QTreeWidgetItem *item = m_ui->treeWidget->currentItem();
        if (!item)
            return;

        if (item->text(1).toInt() == value)
            return;

        QFont italicFont = item->font(0);
        italicFont.setItalic(true);

        item->setText(1, QString::number(value));
        item->setFont(0, italicFont);
    });

    // add a new packaging
    connect(m_ui->addButton, &QToolButton::clicked, this, [this]()
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->treeWidget);

        QFont italicFont = item->font(0);
        italicFont.setItalic(true);

        item->setText(0, tr("Unnamed"));
        item->setData(0, IdRole, -1);
        item->setData(0, OrdersRole, 0);
        item->setFont(0, italicFont);
        item->setText(1, "100");

        m_ui->treeWidget->setCurrentItem(item);
    });

    // remove and save id if existing
    connect(m_ui->removeButton, &QToolButton::clicked, this, [this]()
    {
        QTreeWidgetItem *item = m_ui->treeWidget->currentItem();
        if (!item)
            return;

        const int ordersWithPackaging = item->data(0, OrdersRole).toInt();
        if (ordersWithPackaging > 0) {
            QMessageBox::warning(this, tr("Can't remove"), tr("This packaging can't be removed, there are orders using it."));
            return;
        }

        const int id = item->data(0, IdRole).toInt();
        if (id > 0)
            m_removedIds << id;

        delete item;
    });
}

PackagingSettingsPage::~PackagingSettingsPage()
{
    delete m_ui;
    m_ui = nullptr;
}

void PackagingSettingsPage::init(OrderManager *orderMgr, SqlManager *sqlMgr)
{
    Q_UNUSED(orderMgr);

    m_sqlMgr = sqlMgr;
}

QString PackagingSettingsPage::pageName() const
{
    return tr("Packaging");
}

QPixmap PackagingSettingsPage::pageIcon() const
{
    return QPixmap(":/res/icons/package.png");
}

void PackagingSettingsPage::readSettings(const SharedData &shared)
{
    Q_UNUSED(shared);

    const QList<Packaging> packagings = m_sqlMgr->packagings();
    for (const Packaging &pack : packagings) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->treeWidget);
        item->setText(0, pack.name);
        item->setData(0, IdRole, pack.id);
        item->setData(0, UrlRole, pack.restockUrl);
        item->setData(0, OrdersRole, m_sqlMgr->ordersWithPackaging(pack.id));
        item->setText(1, QString::number(pack.stock));
    }
}

void PackagingSettingsPage::writeSettings(SharedData &shared)
{
    Q_UNUSED(shared);

    for (const int id : m_removedIds)
         m_sqlMgr->removePackaging(id);

    for (int i = 0; i < m_ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_ui->treeWidget->topLevelItem(i);

        const bool isModified = item->font(0).italic();
        if (!isModified)
            continue;

        const int id = item->data(0, IdRole).toInt();
        if (id == 0)
            continue;

        //                          id, name,          stock,                 restock url
        m_sqlMgr->updatePackaging({ id, item->text(0), item->text(1).toInt(), item->data(0, UrlRole).toString() });
    }
}
