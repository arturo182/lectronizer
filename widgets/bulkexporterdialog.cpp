#include "bulkexporterdialog.h"
#include "ordermanager.h"
#include "shareddata.h"
#include "sqlmanager.h"
#include "ui_bulkexporterdialog.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTreeWidgetItem>

// TODO: When any of the checkboxes or columns are changed and a custom preset is selected, save changes to the preset
// TODO: Implement Preview
// TODO: Implement Export

// Only add new items at the end (before LastValue)
// First items coincide with ModelColumn cause maybe we merge them one day
enum class ColumnId
{
    Id = 0,
    CreationDate,
    OrderTotal,
    Items,
    Organization,
    Country,
    ShippingMethod,
    Status,
    UpdatedDate,
    FulfilledDate,
    Weight,
    FulfillUntil,
    CustomerNote,
    YourNote,
    Currency,
    ShippingTotal,
    Tax,
    PlatformFees,
    PaymentFees,
    DiscountTotal,
    DiscountCodes,
    Payout,
    PaymentProvider,
    PaymentReference,
    Packaging,
    TrackingNumber,
    TrackingUrl,
    FirstName,
    LastName,
    Address1,
    Address2,
    PostalCode,
    City,
    Phone,
    Email,

    LastValue,
};

struct ColumnInfo
{
    ColumnId id;
    QString name;
    QString csvName;
};

static QList<ColumnInfo> columnInfos =
{
    { ColumnId::Id,                 "ID",               "id" },
    { ColumnId::Status,             "Status",           "status" },

    { ColumnId::CreationDate,       "Created",          "created_at" },
    { ColumnId::UpdatedDate,        "Updated",          "updated_at" },
    { ColumnId::FulfilledDate,      "Fulfilled",        "fulfilled_at" },

    { ColumnId::CustomerNote,       "Customer Note",    "customer_note" },
    { ColumnId::YourNote,           "Your Note",        "your_note" },

    { ColumnId::Items,              "Items",            "items" },

    // Billing
    { ColumnId::Currency,           "Currency",         "currency" },

    { ColumnId::OrderTotal,         "Order Total",      "total" },
    { ColumnId::ShippingTotal,      "Shipping Total",   "shipping_cost" },
    { ColumnId::Tax,                "Tax Total",        "tax_total" },
    { ColumnId::PlatformFees,       "Platform Fees",    "platform_fees" },
    { ColumnId::PaymentFees,        "Payment Fees",     "payment_fees" },

    { ColumnId::DiscountTotal,      "Discount Total",   "discount_total" },
    { ColumnId::DiscountCodes,      "Discount Codes",   "discount_codes" },

    { ColumnId::Payout,             "Payout",           "payout" },

    { ColumnId::PaymentProvider,    "Payment Provider", "payment_provider" },
    { ColumnId::PaymentReference,   "Payment Reference","payment_reference" },

    // Shipping
    { ColumnId::FulfillUntil,       "Fulfill Until",    "fulfill_until" },
    { ColumnId::Weight,             "Weight",           "weight" },
    { ColumnId::Packaging,          "Packaging",        "packaging" },
    { ColumnId::ShippingMethod,     "Shipping Method",  "shipping_method" },

    { ColumnId::TrackingNumber,     "Tracking #",       "tracking_code" },
    { ColumnId::TrackingUrl,        "Tracking Url",     "tracking_url" },

    { ColumnId::FirstName,          "First Name",       "first_name" },
    { ColumnId::LastName,           "Last Name",        "last_name" },
    { ColumnId::Organization,       "Organization",     "organization" },
    { ColumnId::Address1,           "Address 1",        "street" },
    { ColumnId::Address2,           "Address 2",        "street_extension" },
    { ColumnId::PostalCode,         "Postal Code",      "postal_code" },
    { ColumnId::City,               "City",             "city" },
    { ColumnId::Country,            "Country",          "country" },
    { ColumnId::Phone,              "Phone",            "customer_phone" },
    { ColumnId::Email,              "Email",            "customer_email" },
};

static const QList<ColumnId> defaultPreset =
{
    ColumnId::Id,
    ColumnId::CreationDate,
    ColumnId::Status,
    ColumnId::OrderTotal,
    ColumnId::Currency,
    ColumnId::Items,
    ColumnId::ShippingMethod,
    ColumnId::FulfilledDate,
    ColumnId::Weight,
    ColumnId::CustomerNote,
    ColumnId::YourNote,
    ColumnId::TrackingNumber,
    ColumnId::FirstName,
    ColumnId::LastName,
    ColumnId::Organization,
    ColumnId::Address1,
    ColumnId::Address2,
    ColumnId::PostalCode,
    ColumnId::City,
    ColumnId::Country,
    ColumnId::Phone,
    ColumnId::Email,
};

static const int IdRole         = Qt::UserRole + 0;
static const int CsvNameRole    = Qt::UserRole + 1;

static QVariant columnsToVariant(const QList<ColumnId> &columns)
{
    QVariantList variantList;
    for (const ColumnId &id : columns)
        variantList << (int)id;

    return variantList;
}

static QList<ColumnId> variantToColumns(const QVariantList &variantList)
{
    QList<ColumnId> columns;
    for (const QVariant &var : variantList)
        columns << (ColumnId)var.toInt();

    return columns;
}

BulkExporterDialog::BulkExporterDialog(OrderManager *orderMgr, QSortFilterProxyModel *proxyModel, SqlManager *sqlMgr, SharedData *shared, QWidget *parent)
    : QDialog{parent}
    , m_orderMgr{orderMgr}
    , m_proxyModel{proxyModel}
    , m_shared{shared}
    , m_sqlMgr{sqlMgr}
    , m_ui{new Ui::BulkExporterDialog}
{
    m_ui->setupUi(this);

    m_ui->buttonBox->button(QDialogButtonBox::Save)->setText(tr("Export"));

    for (int i = 0; i < m_proxyModel->rowCount(); ++i)
        m_orderIds << m_proxyModel->index(i, 0).data().toInt();

    if (m_orderIds.isEmpty())
        QMessageBox::information(this, tr("No orders?"), tr("The Exporter uses orders listed in the main window, use the filter and search functionality to narrow down which orders you want to export."));

    for (const ColumnInfo &info : columnInfos) {
        QTreeWidgetItem *columnItem = new QTreeWidgetItem(m_ui->columnTreeWidget);

        columnItem->setText(0, info.name);
        columnItem->setData(0, IdRole, (int)info.id);
        columnItem->setData(0, CsvNameRole, info.csvName);
        columnItem->setCheckState(0, Qt::Unchecked);
    }

    // keep before addPreset() and readSettings()
    connectSignals();

    addPreset(tr("Default"), defaultPreset);

    readSettings();
    updateColumnButtons();
}

BulkExporterDialog::~BulkExporterDialog()
{
    delete m_ui;
    m_ui = nullptr;
}

void BulkExporterDialog::done(int r)
{
    // "Export"
    if (r == QDialog::Accepted) {
        const QString fileName = QFileDialog::getSaveFileName(this, tr("Export orders"), m_lastSaveDir, tr("CSV files (*.csv)"));
        if (fileName.isEmpty())
            return;

        m_lastSaveDir = QFileInfo(fileName).absolutePath();

        QFile file(fileName);
        if (!file.open(QFile::ReadWrite)) {
            QMessageBox::critical(this, tr("Failed to open file"), file.errorString());
            return;
        }

        const QString csv = generateCsv(false);
        file.write(csv.toUtf8());
        file.close();

        const QMessageBox::StandardButton button = QMessageBox::information(this, tr("Export completed"), tr("The orders have been exported. Click Open to open the directory or Ok to close the dialog."), QMessageBox::Open | QMessageBox::Ok);
        if (button == QMessageBox::Open) {
            const QString nativeFilePath = QDir::toNativeSeparators(fileName);

#ifdef Q_OS_WIN
            QProcess::startDetached("explorer.exe", { "/select,", nativeFilePath });
#elif defined(Q_OS_MAC)
            QProcess::execute("/usr/bin/osascript", { "-e", "tell application \"Finder\" to reveal POSIX file \"" + nativeFilePath + "\"" });
            QProcess::execute("/usr/bin/osascript", { "-e", "tell application \"Finder\" to activate" });
#else
            QDesktopServices::openUrl(QUrl::fromLocalFile(nativeFilePath));
#endif
        }

        return;
    }

    writeSettings();

    QDialog::done(r);
}

void BulkExporterDialog::closeEvent(QCloseEvent *event)
{
    writeSettings();

    QWidget::closeEvent(event);
}

void BulkExporterDialog::readSettings()
{
    QSettings set;

    set.beginGroup("BulkExporter");
    restoreGeometry(set.value("geometry").toByteArray());
    setWindowState(Qt::WindowStates(set.value("state").toInt()));
    m_lastSaveDir = set.value("last_save_dir").toString();

    for (const QString &name : set.childGroups()) {
        set.beginGroup(name);

        const QVariantList variantList = set.value("columns").toList();
        addPreset(name, variantToColumns(variantList));

        set.endGroup();
    }

    const QString preset = set.value("preset", tr("Default")).toString();
    for (int i = 1; i < m_ui->presetComboBox->count(); ++i) { // Skip "New"
        if (m_ui->presetComboBox->itemText(i) != preset)
            continue;

        m_ui->presetComboBox->setCurrentIndex(i);

        break;
    }
}

void BulkExporterDialog::writeSettings() const
{
    QSettings set;

    set.beginGroup("BulkExporter");
    set.setValue("geometry", saveGeometry());
    set.setValue("state", (int)windowState());
    set.setValue("last_save_dir", m_lastSaveDir);
    set.setValue("preset", m_ui->presetComboBox->currentText());

    // Delete all, this way removed presets are not preserved
    for (const QString &name : set.childGroups())
        set.remove(name);

    // Skip "New" and "Default"
    QComboBox *presetCombo = m_ui->presetComboBox;
    for (int i = 2; i < presetCombo->count(); ++i) {
        set.beginGroup(presetCombo->itemText(i));
        set.setValue("columns", presetCombo->itemData(i).toList());
        set.endGroup();
    }
}

void BulkExporterDialog::connectSignals()
{
    // Preview

    // Columns
    connect(m_ui->columnTreeWidget, &QTreeWidget::itemSelectionChanged, this, &BulkExporterDialog::updateColumnButtons);
    connect(m_ui->columnTreeWidget, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem*)
    {
        updateCurrentPreset();
        updatePreview();
    });

    connect(m_ui->columnUpButton, &QToolButton::clicked, this, [this]()
    {
        QTreeWidget *treeWidget = m_ui->columnTreeWidget;
        QTreeWidgetItem *currentItem = treeWidget->currentItem();
        const int currentIdx = treeWidget->indexOfTopLevelItem(currentItem);
        if (!currentItem || (currentIdx < 1))
            return;

        treeWidget->takeTopLevelItem(currentIdx);
        treeWidget->insertTopLevelItem(currentIdx - 1, currentItem);
        treeWidget->setCurrentItem(currentItem);

        updateColumnButtons();
        updateCurrentPreset();
        updatePreview();
    });

    connect(m_ui->columnDownButton, &QToolButton::clicked, this, [this]()
    {
        QTreeWidget *treeWidget = m_ui->columnTreeWidget;
        QTreeWidgetItem *currentItem = treeWidget->currentItem();
        const int currentIdx = treeWidget->indexOfTopLevelItem(currentItem);
        const int itemCount = treeWidget->topLevelItemCount();
        if (!currentItem || (currentIdx >= itemCount - 1))
            return;

        treeWidget->takeTopLevelItem(currentIdx);
        treeWidget->insertTopLevelItem(currentIdx + 1, currentItem);
        treeWidget->setCurrentItem(currentItem);

        updateColumnButtons();
        updateCurrentPreset();
        updatePreview();
    });

    // Presets
    connect(m_ui->presetComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx)
    {
        const bool isNewPreset = (idx == 0);
        const bool isDefault = (idx == 1);

        m_ui->deletePresetButton->setEnabled(!isNewPreset && !isDefault);

        if (isNewPreset) {
            const QList<ColumnId> columns = currentColumnList();

            if (columns.isEmpty()) {
                QMessageBox::warning(this, tr("No columns selected"), tr("You need to select at least one column before saving the preset."));

                // Back to default
                m_ui->presetComboBox->setCurrentIndex(1);
                return;
            }

            const QString name = QInputDialog::getText(this, tr("New Preset"), tr("Provide the preset name:"));
            if (name.isEmpty()) {
                // Back to default
                m_ui->presetComboBox->setCurrentIndex(1);
                return;
            }

            addPreset(name, columns);
        } else {
            const QVariantList variantList = m_ui->presetComboBox->currentData().toList();
            applyPreset(variantToColumns(variantList));
        }
    });

    connect(m_ui->deletePresetButton, &QToolButton::pressed, this, [this]()
    {
        const int idx = m_ui->presetComboBox->currentIndex();
        if (idx < 2)
            return;

        // Switch to "Default"
        m_ui->presetComboBox->setCurrentIndex(1);

        m_ui->presetComboBox->removeItem(idx);
    });
}

void BulkExporterDialog::addPreset(const QString &name, const QList<ColumnId> &columns)
{
    m_ui->presetComboBox->addItem(name, columnsToVariant(columns));
    m_ui->presetComboBox->setCurrentIndex(m_ui->presetComboBox->count() - 1);
}

void BulkExporterDialog::applyPreset(const QList<ColumnId> &columns)
{
    QHash<int, QTreeWidgetItem*> items;

    // Remove all
    while (m_ui->columnTreeWidget->topLevelItemCount()) {
        QTreeWidgetItem *item = m_ui->columnTreeWidget->takeTopLevelItem(0);
        const ColumnId id = (ColumnId)item->data(0, IdRole).toInt();

        items.insert((int)id, item);
    }

    // Add the selected ones in order, checked
    for (const ColumnId &id : columns) {
        QTreeWidgetItem *item = items.take((int)id);
        item->setCheckState(0, Qt::Checked);

        m_ui->columnTreeWidget->addTopLevelItem(item);
    }

    // Add the rest, unchecked
    QHashIterator<int, QTreeWidgetItem*> it(items);
    while (it.hasNext()) {
        it.next();

        QTreeWidgetItem *item = it.value();
        item->setCheckState(0, Qt::Unchecked);

        m_ui->columnTreeWidget->addTopLevelItem(item);
    }

    updatePreview();
}

void BulkExporterDialog::updateCurrentPreset()
{
    const int preset = m_ui->presetComboBox->currentIndex();
    if (preset < 2)
        return;

    m_ui->presetComboBox->setItemData(preset, columnsToVariant(currentColumnList()));
}

void BulkExporterDialog::updatePreview()
{
    const QString csvPreview = generateCsv(true);
    m_ui->previewTextEdit->setPlainText(csvPreview);
}

QString BulkExporterDialog::generateCsv(const bool preview)
{
    QTreeWidget *columnTree = m_ui->columnTreeWidget;

    char sep = ',';
    switch (m_shared->csvSeparator) {
    case SepComma:      sep = ',';  break;
    case SepSemicolon:  sep = ';';  break;
    case SepTab:        sep = '\t'; break;
    default:
        break;
    }

    QList<ColumnId> columns;
    QStringList columnNames;

    for (int i = 0; i < columnTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *columnItem = columnTree->topLevelItem(i);
        const ColumnId columnId = (ColumnId)columnItem->data(0, IdRole).toInt();
        const QString csvName = columnItem->data(0, CsvNameRole).toString();
        const bool isChecked = (columnItem->checkState(0) == Qt::Checked);

        if (!isChecked)
            continue;

        columns << columnId;
        columnNames << csvName;
    }

    QString csv;
    csv += columnNames.join(sep) + "\r\n";

    int i = 0;
    for (const int orderId : m_orderIds) {
        QStringList values;

        for (const ColumnId &column : columns) {
            const QVariant value = valueForOrderColumn(orderId, column);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
            switch (value.typeId()) {
#else
            switch ((QMetaType::Type)value.type()) {
#endif
            case QMetaType::Int:        values << QString::number(value.toInt());                           break;
            case QMetaType::Double:     values << QString::number(value.toDouble(), 'f', 2);                break;
            case QMetaType::QString:    values << "\"" + value.toString().replace("\"", "\\\"") + "\"";     break;
            case QMetaType::QDateTime:  values << value.toDateTime().toString(Qt::ISODate);                 break;

            default:
                qDebug() << "Unknown value type" << value.typeName() << value;
                break;
            }
        }

        csv += values.join(sep) + "\r\n";

        i += 1;
        if (preview && (i == 25))
            break;
    }

    return csv;
}

QList<ColumnId> BulkExporterDialog::currentColumnList() const
{
    QList<ColumnId> columns;

    for (int i = 0; i < m_ui->columnTreeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_ui->columnTreeWidget->topLevelItem(i);
        if (item->checkState(0) == Qt::Unchecked)
            continue;

        columns << (ColumnId)item->data(0, IdRole).toInt();
    }

    return columns;
}

QVariant BulkExporterDialog::valueForOrderColumn(const int id, const ColumnId &column) const
{
    const Order &order = m_orderMgr->order(id);

    double discountTotal = 0;
    for (const Item &item : order.items)
        discountTotal = item.discount * item.qty;

    QString packaging = tr("Unpackaged");
    for (const Packaging &pack : m_sqlMgr->packagings()) {
        if (pack.id == order.packaging) {
            packaging = pack.name;
            break;
        }
    }

    // By default payout is in local currency, this gives us EUR
    const double payout = order.total - order.lectronzFee - order.paymentFee - order.tax.collected;

    switch (column) {
    case ColumnId::Id:              return order.id;
    case ColumnId::CreationDate:    return order.createdAt;
    case ColumnId::OrderTotal:      return order.total;
    case ColumnId::Items:           return order.itemListing();
    case ColumnId::Organization:    return order.shipping.address.organization;
    case ColumnId::Country:         return order.shipping.address.country;
    case ColumnId::ShippingMethod:  return order.shipping.method;
    case ColumnId::Status:          return order.statusString();
    case ColumnId::UpdatedDate:     return order.updatedAt;
    case ColumnId::FulfilledDate:   return order.fulfilledAt;
    case ColumnId::Weight:          return order.weight.total;
    case ColumnId::FulfillUntil:    return order.fulfillUntil;
    case ColumnId::CustomerNote:    return order.customerNote;
    case ColumnId::YourNote:        return order.note;
    case ColumnId::Currency:        return order.currency;
    case ColumnId::ShippingTotal:   return order.shipping.cost;
    case ColumnId::Tax:             return order.tax.total;
    case ColumnId::PlatformFees:    return order.lectronzFee;
    case ColumnId::PaymentFees:     return order.paymentFee;
    case ColumnId::DiscountTotal:   return discountTotal;
    case ColumnId::DiscountCodes:   return order.discountCodes.join(',');
    case ColumnId::Payout:          return payout;
    case ColumnId::PaymentProvider: return order.payment.provider;
    case ColumnId::PaymentReference:return order.payment.reference;
    case ColumnId::Packaging:       return packaging;
    case ColumnId::TrackingNumber:  return order.tracking.code;
    case ColumnId::TrackingUrl:     return order.tracking.url;
    case ColumnId::FirstName:       return order.shipping.address.firstName;
    case ColumnId::LastName:        return order.shipping.address.lastName;
    case ColumnId::Address1:        return order.shipping.address.street;
    case ColumnId::Address2:        return order.shipping.address.streetExtension;
    case ColumnId::PostalCode:      return order.shipping.address.postalCode;
    case ColumnId::City:            return order.shipping.address.city;
    case ColumnId::Phone:           return order.customerPhone;
    case ColumnId::Email:           return order.customerEmail;
    default:
        break;
    }

    return QVariant();
}

void BulkExporterDialog::updateColumnButtons()
{
    QTreeWidget *treeWidget = m_ui->columnTreeWidget;
    QTreeWidgetItem *currentItem = treeWidget->currentItem();
    const int currentIdx = treeWidget->indexOfTopLevelItem(currentItem);
    const int itemCount = treeWidget->topLevelItemCount();
    const bool hasSelection = (currentIdx != -1);

    m_ui->columnUpButton->setEnabled(hasSelection && (currentIdx > 0));
    m_ui->columnDownButton->setEnabled(hasSelection && (currentIdx < itemCount - 1));
}
