#include "orderdetailswidget.h"
#include "shareddata.h"
#include "ui_orderdetailswidget.h"
#include "utils.h"

#include <QClipboard>
#include <QDesktopServices>
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

    // Mark shipped
    connect(m_ui->shippingSubmitButton, &QPushButton::clicked, this, [&]()
    {
       QMessageBox::information(this, tr("Not implemented"), tr("Not implemented yet, sorry"));
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

void OrderDetailsWidget::setOrder(const Order &order)
{
    m_order = order;

    if (!parent())
        setWindowTitle(tr("Order details - #%1").arg(order.id));

    const Address &address = order.shipping.address;

    // Header
    m_ui->orderNumberLabel->setText(tr("<a href='%1'>Order #%2</a>").arg(order.editUrl(), QString::number(order.id)));
    m_ui->orderStatusLabel->setText(tr("Status: %1").arg(order.statusString()));
    m_ui->createdAtLabel->setText(tr("Created: %1").arg(friendlyDate(order.createdAt)));
    m_ui->updatedAtLabel->setText(tr("Updated: %1").arg(friendlyDate(order.updatedAt)));

    // Address
    m_ui->addressNameEdit->setText(address.firstName + " " + address.lastName);
    m_ui->addressOrgEdit->setText(address.organization);
    m_ui->address1Edit->setText(address.street);
    m_ui->address2Edit->setText(address.streetExtension);
    m_ui->addressCityEdit->setText(QString("%1%2%3").arg(address.city, address.state.isEmpty() ? "" : ", ", address.state));
    m_ui->addressZipEdit->setText(address.postalCode);
    m_ui->addressCountryEdit->setText(address.country);
    m_ui->addressPhoneEdit->setText(order.customerPhone);
    m_ui->addressEmailEdit->setText(order.customerEmail);

    // Items
    QFont boldFont = m_ui->itemsTreeWidget->font();
    boldFont.setBold(true);

    // TODO: use custom delegate and draw options smaller under
    m_ui->itemsTreeWidget->clear();
    for (const Item &orderItem : order.items) {
        QTreeWidgetItem *itemItem = new QTreeWidgetItem(m_ui->itemsTreeWidget);
        itemItem->setText(0, orderItem.product.name + " (" + orderItem.product.sku + ")");
        itemItem->setFont(0, boldFont);
        itemItem->setText(1, QString::number(orderItem.qty));
        itemItem->setText(2, QString::number(orderItem.price) + " " + order.currency);
        itemItem->setText(3, QString::number(orderItem.qty * orderItem.price, 'g', 4) + " " + order.currency);

        for (int i = 0; i < orderItem.options.count(); ++i) {
            QTreeWidgetItem *optionItem = new QTreeWidgetItem(itemItem);
            optionItem->setText(0, orderItem.options[i].name + " = " + orderItem.options[i].choice);
        }
    }
    m_ui->itemsTreeWidget->expandAll();

    // Totals
    QString totalText;
    totalText += tr("Subtotal %1 %2\n")
            .arg(order.subtotal, 7, 'f', 2)
            .arg(order.currency);
    totalText += tr("Shipping (%1 Tax/VAT) %2 %3\n")
            .arg(order.tax.appliesToShipping ? "included in" : "excluded from")
            .arg(order.shipping.cost, 7, 'f', 2)
            .arg(order.currency);
    totalText += tr("VAT (%1 %) %2 %3\n")
            .arg(order.tax.rate).arg(order.tax.total, 7, 'f', 2)
            .arg(order.currency);
    totalText += tr("Total %1 %2")
            .arg(order.total, 7, 'f', 2)
            .arg(order.currency);

    if (m_shared->targetCurrency != "EUR" && (m_shared->currencyRates.size() > 1))
        totalText += QString("\n%1 %2")
                        .arg(order.total * m_shared->currencyRates[m_shared->targetCurrency], 7, 'f', 2)
                        .arg(m_shared->targetCurrency);

    m_ui->itemsTotalLabel->setText(totalText);

    // Shipping
    m_ui->shippingWeightValueLabel->setText(tr("%1 %2").arg(order.calcWeight(), 0, 'f', 1).arg(order.weight.unit));
    m_ui->shippingTrackingRequiredLabel->setText(order.tracking.required ? tr("Required") : tr("Not required"));
    m_ui->shippingTrackingRequiredLabel->setStyleSheet(order.tracking.required ? "font-weight: bold; color: red;" : "");
    m_ui->shippingMethodValueLabel->setText(order.shipping.method);
    m_ui->shippingSubmitButton->setDisabled(order.fulfilledAt.isValid());
    m_ui->shippingSubmitButton->setText(order.fulfilledAt.isValid() ? tr("Shipped %1").arg(friendlyDate(order.fulfilledAt)) : tr("Mark Shipped"));

    // Billing
    QString billingText;
    billingText += tr("Total %1 %2\n")
            .arg(order.total, 7, 'f', 2)
            .arg(order.currency);
    billingText += tr("Lectronz fee (%1%) %2 %3\n")
            .arg(order.lectronzFee * 100 / order.total, 0, 'f', 2)
            .arg(order.lectronzFee, 7, 'f', 2)
            .arg(order.currency);
    billingText += tr("Payment proc. fee (%1%) %2 %3\n")
            .arg(order.paymentFee * 100 / order.total, 0, 'f', 2)
            .arg(order.paymentFee, 7, 'f', 2)
            .arg(order.currency);
    billingText += tr("Tax to collect %1 %2\n")
            .arg(order.tax.total, 7, 'f', 2)
            .arg(order.currency);
    billingText += tr("Payout %1 %2")
            .arg(order.total - order.lectronzFee - order.paymentFee, 7, 'f', 2)
            .arg(order.currency);

    if (m_shared->targetCurrency != "EUR" && (m_shared->currencyRates.size() > 1))
        billingText += QString("\n%1 %2")
                        .arg(order.payout, 7, 'f', 2)
                        .arg(m_shared->targetCurrency);

    m_ui->billingLabel->setText(billingText);
    m_ui->billingLinkLabel->setText(QString("<a href='https://dashboard.stripe.com/payments/%1'>See payment on Stripe</a>").arg(order.payment.reference));
}

void OrderDetailsWidget::updateOrderButtons(const int row, const int rowCount)
{
    m_ui->prevOrderButton->setEnabled(row > 0);
    m_ui->nextOrderButton->setEnabled(row < (rowCount - 1));
    m_ui->orderPosLabel->setText(QString("%1/%2").arg(row + 1).arg(rowCount));
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
}
