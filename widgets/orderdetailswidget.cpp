#include "markshippeddialog.h"
#include "orderdetailswidget.h"
#include "orderitemdelegate.h"
#include "ordermanager.h"
#include "shareddata.h"
#include "sqlmanager.h"
#include "ui_orderdetailswidget.h"
#include "utils.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QUrl>

OrderDetailsWidget::OrderDetailsWidget(const bool standalone, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::OrderDetailsWidget)
{
    m_ui->setupUi(this);

    m_ui->addressHeader->setContainer(m_ui->addressContainer);
    m_ui->itemsHeader->setContainer(m_ui->itemsContainer);
    m_ui->shippingHeader->setContainer(m_ui->shippingContainer);
    m_ui->billingHeader->setContainer(m_ui->billingContainer);
    m_ui->notesHeader->setContainer(m_ui->noteTextEdit);

    if (standalone) {
        m_ui->collapseButton->hide();
        m_ui->orderPosLabel->hide();
        m_ui->prevOrderButton->hide();
        m_ui->nextOrderButton->hide();

        QSettings set;
        set.beginGroup("OrderDetails");
        readSettings(set);
    }

    // This seems to reset in Designer from time to time (bug?) so just set from code
    QFont monospaceFont = font();
    monospaceFont.setFamily("Courier New");
    m_ui->itemsTotalLabel->setFont(monospaceFont);
    m_ui->billingLabel->setFont(monospaceFont);

    // Copy full shipping address
    connect(m_ui->addressCopyAllButton, &QPushButton::clicked, this, [&]() { m_order.copyFullAddress(); });

    // Packaging order
    connect(m_ui->shippingPackagingComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]()
    {
        const int packId = m_ui->shippingPackagingComboBox->currentData().toInt();
        m_orderMgr->setPackaging(m_order.id, packId);
    });

    // Mark shipped
    connect(m_ui->shippingSubmitButton, &QPushButton::clicked, this, [&]()
    {
        if (m_order.tracking.required) {
            MarkShippedDialog dlg(m_order, m_shared, this);
            if (dlg.exec() == QDialog::Accepted)
                m_orderMgr->markShipped(m_order.id, dlg.trackingNo(), dlg.trackingUrl());
        } else {
            m_orderMgr->markShipped(m_order.id);
        }
    });

    // Process Prev/Next order buttons
    connect(m_ui->prevOrderButton, &QPushButton::clicked, this, &OrderDetailsWidget::processOrderButton);
    connect(m_ui->nextOrderButton, &QPushButton::clicked, this, &OrderDetailsWidget::processOrderButton);

    // Collapse order detail widget
    connect(m_ui->collapseButton, &QPushButton::clicked, this, &OrderDetailsWidget::hideRequested);

    // Invoice buttons
    connect(m_ui->customerInvoiceButton, &QPushButton::clicked, this, [this]()
    {
        QDesktopServices::openUrl(QUrl(m_order.customerInvoiceUrl()));
    });
    connect(m_ui->sellerInvoiceButton, &QPushButton::clicked, this, [this]()
    {
        QDesktopServices::openUrl(QUrl(m_order.supplierInvoiceUrl()));
    });

    connect(m_ui->noteTextEdit, &QPlainTextEdit::textChanged, this, [this]()
    {
        const QString note = m_ui->noteTextEdit->toPlainText();

        Order &order = m_orderMgr->order(m_order.id);
        if (order.note == note)
            return;

        order.note = note;
        m_sqlMgr->save(order);
    });
}

OrderDetailsWidget::~OrderDetailsWidget()
{
    delete m_ui;
    m_ui = nullptr;
}

void OrderDetailsWidget::setSharedData(SharedData *shared)
{
    m_shared = shared;
}

void OrderDetailsWidget::setOrderManager(OrderManager *orderMgr)
{
    m_orderMgr = orderMgr;

    m_ui->itemsTreeWidget->setItemDelegateForColumn(0, new OrderItemDelegate(m_orderMgr, this));

    connect(m_orderMgr, &OrderManager::orderUpdated, this, [this](const Order &order)
    {
        if (order.id == m_order.id)
            setOrder(order);

        // update packaging combo labels, regardless which order changed
        for (const Packaging &pack : m_sqlMgr->packagings()) {
            // start at 2 cause No packaging, and Default packaging have no stock tracking
            for (int i = 2; i < m_ui->shippingPackagingComboBox->count(); ++i) {
                const int itemPackId = m_ui->shippingPackagingComboBox->itemData(i).toInt();

                if (itemPackId != pack.id)
                    continue;

                m_ui->shippingPackagingComboBox->setItemText(i, tr("%1 (%2 left)").arg(pack.name).arg(pack.stock));
                break;
            }
        }
    });
}

void OrderDetailsWidget::setSqlManager(SqlManager *sqlMgr)
{
    m_sqlMgr = sqlMgr;

    m_ui->shippingPackagingComboBox->addItem(tr("Not packaged"), -1);
    m_ui->shippingPackagingComboBox->addItem(tr("Default packaging"), 0);

    for (const Packaging &pack : m_sqlMgr->packagings())
        m_ui->shippingPackagingComboBox->addItem(tr("%1 (%2 left)").arg(pack.name).arg(pack.stock), pack.id);
}

void OrderDetailsWidget::setOrder(const Order &order)
{
    m_order = order;

    updateOrderDetails();
}

void OrderDetailsWidget::updateOrderButtons(const int row, const int rowCount)
{
    m_ui->prevOrderButton->setEnabled(row > 0);
    m_ui->nextOrderButton->setEnabled(row < (rowCount - 1));
    m_ui->orderPosLabel->setText(QString("%1/%2").arg(row + 1).arg(rowCount));
}

void OrderDetailsWidget::updateOrderDetails()
{
    const Address &address = m_order.shipping.address;

    if (!parent())
        setWindowTitle(tr("Order details - #%1 - %2").arg(m_order.id).arg(address.firstName + " " + address.lastName));

    // Header
    m_ui->orderNumberLabel->setText(tr("<a href='%1'>Order #%2</a>").arg(m_order.editUrl(), QString::number(m_order.id)));
    m_ui->orderStatusLabel->setText(tr("Status: %1").arg(m_order.statusString()));
    m_ui->createdAtLabel->setText(tr("Created: %1").arg(textDate(m_order.createdAt, *m_shared)));
    m_ui->updatedAtLabel->setText(tr("Updated: %1").arg(textDate(m_order.updatedAt, *m_shared)));

    // Address
    m_ui->addressNameEdit->setText(address.firstName + " " + address.lastName);
    m_ui->addressOrgEdit->setText(address.organization);
    m_ui->address1Edit->setText(address.street);
    m_ui->address2Edit->setText(address.streetExtension);
    m_ui->addressCityEdit->setText(QString("%1%2%3").arg(address.city, address.state.isEmpty() ? "" : ", ", shortenUsState(address.country, address.state)));
    m_ui->addressZipEdit->setText(address.postalCode);
    m_ui->addressCountryEdit->setText(address.country);
    m_ui->addressPhoneEdit->setText(sanitizePhoneNumber(m_order.customerPhone, address.country, *m_shared));
    m_ui->addressEmailEdit->setText(m_order.customerEmail);

    // Items
    QFont boldFont = m_ui->itemsTreeWidget->font();
    boldFont.setBold(true);

    m_ui->itemsTreeWidget->clear();
    for (int i = 0; i < m_order.items.size(); ++i) {
        const Item &orderItem = m_order.items[i];

        QTreeWidgetItem *treeItem = new QTreeWidgetItem(m_ui->itemsTreeWidget);
        treeItem->setData(0, Qt::UserRole + 0, m_order.id);
        treeItem->setData(0, Qt::UserRole + 1, i);
        treeItem->setText(1, QString::number(orderItem.price) + " " + m_order.currency);
        treeItem->setText(2, QString::number(orderItem.qty * orderItem.price, 'g', 4) + " " + m_order.currency);
    }

    // Totals
    QString totalText;
    totalText += tr("Subtotal %1 %2\n")
            .arg(m_order.subtotal, 7, 'f', 2)
            .arg(m_order.currency);
    totalText += tr("Shipping (%1 Tax/VAT) %2 %3\n")
            .arg(m_order.tax.appliesToShipping ? "included in" : "excluded from")
            .arg(m_order.shipping.cost, 7, 'f', 2)
            .arg(m_order.currency);
    totalText += tr("VAT (%1 %) %2 %3\n")
            .arg(m_order.tax.rate).arg(m_order.tax.total, 7, 'f', 2)
            .arg(m_order.currency);
    totalText += tr("Total %1 %2")
            .arg(m_order.total, 7, 'f', 2)
            .arg(m_order.currency);

    if (m_shared->targetCurrency != "EUR" && (m_shared->currencyRates.size() > 1))
        totalText += QString("\n%1 %2")
                        .arg(m_order.total * m_shared->currencyRates[m_shared->targetCurrency], 7, 'f', 2)
                        .arg(m_shared->targetCurrency);

    m_ui->itemsTotalLabel->setText(totalText);

    // Shipping
    m_ui->shippingDeadlineValueLabel->setText(textDate(m_order.fulfillUntil, *m_shared));

    for (int i = 0; i < m_ui->shippingPackagingComboBox->count(); ++i) {
        if (m_order.packaging != m_ui->shippingPackagingComboBox->itemData(i).toInt())
            continue;

        m_ui->shippingPackagingComboBox->setCurrentIndex(i);
        break;
    }
    m_ui->shippingPackagingComboBox->setDisabled(m_order.isShipped() || m_order.isRefunded());

    m_ui->shippingWeightValueLabel->setText(tr("%1 %2").arg(m_order.calcWeight(), 0, 'f', 1).arg(m_order.weight.unit));
    m_ui->shippingTrackingRequiredLabel->setText(m_order.tracking.required ? tr("Required") : tr("Not required"));
    if (!m_order.isShipped()) {
        m_ui->shippingTrackingRequiredLabel->setStyleSheet(m_order.tracking.required ? "font-weight: bold; color: red;" : "");
    } else {
        m_ui->shippingTrackingRequiredLabel->setStyleSheet("");
    }
    m_ui->shippingTrackingNoEdit->setPlaceholderText(m_order.tracking.required ? "Mark Shipped to specify" : "Untracked");
    m_ui->shippingTrackingNoEdit->setText(m_order.tracking.code);
    m_ui->shippingTrackingUrlEdit->setPlaceholderText(m_order.tracking.required ? "Mark Shipped to specify" : "Untracked");
    m_ui->shippingTrackingUrlEdit->setText(m_order.tracking.url);
    m_ui->shippingMethodValueLabel->setText(m_order.shipping.method);
    m_ui->shippingSubmitButton->setDisabled(m_order.isShipped() || m_order.isRefunded());
    if (m_order.isShipped()) {
        m_ui->shippingSubmitButton->setText(tr("Shipped %1").arg(textDate(m_order.fulfilledAt, *m_shared)));
    } else if (m_order.isRefunded()) {
        m_ui->shippingSubmitButton->setText(tr("Order Refunded"));
    } else {
        m_ui->shippingSubmitButton->setText(tr("Mark Shipped"));
    }

    // Billing
    QString billingText;
    billingText += tr("Total %1 %2\n")
            .arg(m_order.total, 7, 'f', 2)
            .arg(m_order.currency);
    billingText += tr("Lectronz fee (%1%) %2 %3\n")
            .arg(m_order.lectronzFee * 100 / m_order.total, 0, 'f', 2)
            .arg(m_order.lectronzFee, 7, 'f', 2)
            .arg(m_order.currency);
    billingText += tr("Payment proc. fee (%1%) %2 %3\n")
            .arg(m_order.paymentFee * 100 / m_order.total, 0, 'f', 2)
            .arg(m_order.paymentFee, 7, 'f', 2)
            .arg(m_order.currency);
    billingText += QString("%1 %2 %3\n")
            .arg(m_order.tax.collected ? tr("Tax collected") : tr("Tax to collect"))
            .arg(m_order.tax.total, 7, 'f', 2)
            .arg(m_order.currency);
    billingText += tr("Payout %1 %2")
            .arg(m_order.total - m_order.lectronzFee - m_order.paymentFee - m_order.tax.collected, 7, 'f', 2)
            .arg(m_order.currency);

    if (m_shared->targetCurrency != "EUR" && (m_shared->currencyRates.size() > 1))
        billingText += QString("\n%1 %2")
                        .arg(m_order.payout, 7, 'f', 2)
                        .arg(m_shared->targetCurrency);

    m_ui->billingLabel->setText(billingText);
    m_ui->billingLinkLabel->setText(QString("<a href='https://dashboard.stripe.com/payments/%1'>See payment on Stripe</a>").arg(m_order.payment.reference));

    // notes
    m_ui->customerNoteTextEdit->setPlainText(m_order.customerNote);
    m_ui->customerNoteLabel->setHidden(m_order.customerNote.isEmpty());
    m_ui->customerNoteTextEdit->setHidden(m_order.customerNote.isEmpty());
    m_ui->noteLabel->setHidden(m_order.customerNote.isEmpty());
    m_ui->noteTextEdit->setPlainText(m_order.note);
}

void OrderDetailsWidget::processOrderButton()
{
    const bool isNext = (sender() == m_ui->nextOrderButton);

    emit orderRequested(isNext ? 1 : -1);
}

void OrderDetailsWidget::readSettings(const QSettings &set)
{
    // separate widget and window settings
    if (set.group().isEmpty()) {
        m_ui->itemsTreeWidget->header()->restoreState(set.value("detailColumns").toByteArray());
        restoreHeaderStates(set.value("detailHeaders").toList());
    } else {
        // inside "OrderDetails" group
        restoreGeometry(set.value("geometry").toByteArray());
        setWindowState(Qt::WindowStates(set.value("state").toInt()));
        restoreHeaderStates(set.value("headers").toList());

        m_ui->itemsTreeWidget->header()->restoreState(set.value("columns").toByteArray());
    }
}

void OrderDetailsWidget::writeSettings(QSettings &set) const
{
    // separate widget and window settings
    if (set.group().isEmpty()) {
        set.setValue("detailColumns", m_ui->itemsTreeWidget->header()->saveState());
        set.setValue("detailHeaders", saveHeaderStates());
    } else {
        // inside "OrderDetails" group
        set.setValue("geometry", saveGeometry());
        set.setValue("state", (int)windowState());
        set.setValue("headers", saveHeaderStates());
        set.setValue("columns", m_ui->itemsTreeWidget->header()->saveState());
    }
}

void OrderDetailsWidget::closeEvent(QCloseEvent *event)
{
    if (!parent()) {
        QSettings set;
        set.beginGroup("OrderDetails");
        writeSettings(set);
    }

    QWidget::closeEvent(event);
}

QVariantList OrderDetailsWidget::saveHeaderStates() const
{
    QVariantList states;
    states << m_ui->addressHeader->isExpanded();
    states << m_ui->itemsHeader->isExpanded();
    states << m_ui->shippingHeader->isExpanded();
    states << m_ui->billingHeader->isExpanded();
    states << m_ui->notesHeader->isExpanded();

    return states;
}

void OrderDetailsWidget::restoreHeaderStates(const QVariantList &states)
{
    const auto restoreHeaderState = [states](const int idx, CollapsibleWidgetHeader *header)
    {
        if (states.size() < (idx + 1))
            return;

        header->setExpanded(states[idx].toBool());
    };

    restoreHeaderState(0, m_ui->addressHeader);
    restoreHeaderState(1, m_ui->itemsHeader);
    restoreHeaderState(2, m_ui->shippingHeader);
    restoreHeaderState(3, m_ui->billingHeader);
    restoreHeaderState(4, m_ui->notesHeader);
}
