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

    if (standalone) {
        m_ui->collapseButton->hide();
        m_ui->orderPosLabel->hide();
        m_ui->prevOrderButton->hide();
        m_ui->nextOrderButton->hide();

        QSettings set;
        set.beginGroup("OrderDetails");
        readSettings(set);
    } else {
        layout()->setContentsMargins(0, 0, 0, 0);
    }

    // This seems to reset in Designer from time to time (bug?) so just set from code
    QFont monospaceFont = font();
    monospaceFont.setFamily("Courier New");
    m_ui->itemsTotalLabel->setFont(monospaceFont);
    m_ui->billingLabel->setFont(monospaceFont);

    // Copy full shipping address
    connect(m_ui->addressCopyAllButton, &QPushButton::clicked, this, &OrderDetailsWidget::copyShippingAddress);

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
        const QUrl url(QString("https://lectronz.com/orders/%1/customer_invoice").arg(m_order.id));
        QDesktopServices::openUrl(url);
    });
    connect(m_ui->sellerInvoiceButton, &QPushButton::clicked, this, [this]()
    {
        const QUrl url(QString("https://lectronz.com/orders/%1/supplier_invoice").arg(m_order.id));
        QDesktopServices::openUrl(url);
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
    m_ui->orderNumberLabel->setText(tr("<a href='https://lectronz.com/orders/%1/edit'>Order #%1</a>").arg(QString::number(order.id)));
    m_ui->orderStatusLabel->setText(tr("Status: %1").arg(order.status));
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

    if (m_shared->targetCurrency != "EUR")
        totalText += QString("\n%1 %2")
                        .arg(order.total * m_shared->currencyRates[m_shared->targetCurrency], 7, 'f', 2)
                        .arg(m_shared->targetCurrency);

    m_ui->itemsTotalLabel->setText(totalText);

    // Shipping
    m_ui->shippingMethodValueLabel->setText(order.shipping.method);
    m_ui->shippingSubmitButton->setDisabled(order.fulfilledAt.isValid()/* || !order.shipping.hasTracking */);
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

    if (m_shared->targetCurrency != "EUR")
        billingText += QString("\n%1 %2")
                        .arg(order.payout, 7, 'f', 2)
                        .arg(m_shared->targetCurrency);

    m_ui->billingLabel->setText(billingText);
    m_ui->billingLinkLabel->setText(QString("<a href='https://dashboard.stripe.com/payments/%1'>See payment on Stripe</a>").arg(order.payment.reference));
}

void OrderDetailsWidget::copyShippingAddress()
{
    const QString name = m_ui->addressNameEdit->text();
    const QString org = m_ui->addressOrgEdit->text();
    const QString address1 = m_ui->address1Edit->text();
    const QString address2 = m_ui->address2Edit->text();
    const QString cityState = m_ui->addressCityEdit->text();
    const QString zip = m_ui->addressZipEdit->text();
    const QString country = m_ui->addressCountryEdit->text();
    const QString phone = m_ui->addressPhoneEdit->text();
    const QString email = m_ui->addressEmailEdit->text();

    QString address;
    address += name + "\n";

    if (!org.isEmpty())
        address += org + "\n";

    address += address1 + "\n";

    if (!address2.isEmpty())
        address += address2 + "\n";

    address += cityState + " " + zip + "\n";
    address += country + "\n";

    if (!phone.isEmpty())
        address += phone + "\n";

    if (!email.isEmpty())
        address += email + "\n";

    if (address.endsWith("\n"))
        address = address.chopped(1);

    qApp->clipboard()->setText(address);

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
    } else {
        restoreGeometry(set.value("geometry").toByteArray());
        setWindowState(Qt::WindowStates(set.value("state").toInt()));

        m_ui->itemsTreeWidget->header()->restoreState(set.value("columns").toByteArray());
    }
}

void OrderDetailsWidget::writeSettings(QSettings &set) const
{
    // separate widget and window settings
    if (set.group().isEmpty()) {
        set.setValue("detailColumns", m_ui->itemsTreeWidget->header()->saveState());
    } else {
        set.setValue("geometry", saveGeometry());
        set.setValue("state", windowState().toInt());
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
