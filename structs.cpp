#include "structs.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

QDebug operator<<(QDebug debug, const Address &a)
{
    QDebugStateSaver saver(debug);
    debug.noquote();
    debug.nospace();

    debug << "Address(" << a.firstName << a.lastName;

    if (!a.organization.isEmpty())
        debug << ", " << a.organization;

    debug << ", " << a.street;

    if (!a.streetExtension.isEmpty())
        debug << ", " << a.streetExtension;

    debug << ", " << a.postalCode << ' ' << a.city;

    if (!a.state.isEmpty())
        debug << ", " << a.state;

    debug << ", " << a.country << ')';

    return debug;
}

QDebug operator<<(QDebug debug, const ItemOption &op)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << "ItemOption(sku = " << op.sku << ", name = " << op.name << ", choice = " << op.choice << ')';

    return debug;
}

QDebug operator<<(QDebug debug, const Item &it)
{
    QDebugStateSaver saver(debug);

    debug.nospace() << "Item(Product(id = " << it.product.id
                    << ", name = " << it.product.name
                    << ", sku = " << it.product.sku
                    << "), options = " << it.options
                    << ", qty = " << it.qty
                    << "price = " << it.price
                    << ')';

    return debug;
}

QDebug operator<<(QDebug debug, const Order &o)
{
    QDebugStateSaver saver(debug);
    debug.noquote();
    debug.nospace();

    debug << "Order(id = " << o.id;
    debug << ", currency = " << o.currency;
    debug << ", subtotal = " << o.subtotal;
    debug << ", taxableAmount = " << o.taxableAmount;
    debug << ", total = " << o.total;
    debug << ", paymentFee = " << o.payout;
    debug << ", lectronzFee = " << o.lectronzFee;
    debug << ", payout = " << o.paymentFee;
    debug << ", payment = { provider = " << o.payment.provider << ", reference = " << o.payment.reference << " }";

    debug << ", createdAt = " << o.createdAt;
    debug << ", updatedAt = " << o.updatedAt;

    if (o.fulfilledAt.isValid())
        debug << ", fulfilledAt = " << o.fulfilledAt;

    debug << ", status = " << o.status;
    debug << ", storeUrl = " << o.storeUrl;
    debug << ", customerEmail = " << o.customerEmail;
    debug << ", items = " << o.items;
    debug << ", tax = { appliesToShipping = " << o.tax.appliesToShipping << ", rate = " << o.tax.rate << ", total = " << o.tax.total << ", collected = " << o.tax.collected <<" }";
    debug << ", billing = ";
    if (o.billing.useShippingAddress) {
        debug << "Use shipping";
    } else {
        debug << o.billing.address;
    }
    debug << ", shipping = { method = " << o.shipping.method << ", cost = " << o.shipping.cost << ", address = " << o.shipping.address << " }";
    debug << ", tracking = { code = " << o.tracking.code << ", url = " << o.tracking.url << "}";

    debug << ')';

    return debug;
}

// QJsonObject wrapper that tracks which keys were read out and prints leftovers on deletion
// This way we can track new fields appearing in the API easily
class JsonObjectTracker
{
    public:
        JsonObjectTracker(const QJsonObject &obj, const QString &funcName)
            : m_funcName{funcName}
            , m_keys{obj.keys()}
            , m_obj{obj}
        {
        }

        ~JsonObjectTracker()
        {
            if (!m_keys.isEmpty())
                qDebug().noquote() << m_funcName << "leftovers:" << m_keys.join(", ");
        }

        QJsonValue value(const QString &key, const bool optional = false)
        {
            if (!m_keys.contains(key) && !optional)
                qDebug() << m_funcName << "KEY NOT FOUND" << key;

            m_keys.removeAll(key);

            return m_obj.value(key);
        }

    private:
        QString m_funcName{};
        QStringList m_keys{};
        const QJsonObject m_obj{};
};

Address parseJsonAddress(const QJsonValue &val)
{
    Address address = {};

    if (!val.isObject())
        return address;

    JsonObjectTracker object(val.toObject(), __func__);
    address.city            = object.value("city").toString();
    address.country         = object.value("country").toString();
    address.countryCode     = object.value("country_code").toString();
    address.firstName       = object.value("first_name").toString();
    address.lastName        = object.value("last_name").toString();
    address.organization    = object.value("organization").toString();
    address.postalCode      = object.value("postal_code").toString();
    address.state           = object.value("state").toString();
    address.street          = object.value("street").toString();
    address.streetExtension = object.value("street_extension").toString();

    return address;
}

ItemOption parseJsonItemOption(const QJsonValue &val)
{
    ItemOption itemOp = {};

    if (!val.isObject())
        return itemOp;

    JsonObjectTracker object(val.toObject(), __func__);
    itemOp.sku     = object.value("sku", true).toString();
    itemOp.name    = object.value("name").toString();
    itemOp.choice  = object.value("choice").toString();

    return itemOp;
}

Item parseJsonItem(const QJsonValue &val)
{
    Item item = {};

    if (!val.isObject())
        return item;

    JsonObjectTracker object(val.toObject(), __func__);
    item.product.id          = object.value("product_id").toInt();
    item.product.name        = object.value("product_name").toString();
    item.product.sku         = object.value("sku").toString();
    item.product.description = object.value("product_description").toString();
    item.qty                 = object.value("quantity").toInt();
    item.price               = object.value("price").toDouble();

    const QJsonArray options = object.value("options").toArray();
    for (const QJsonValue &option : options)
        item.options << parseJsonItemOption(option);

    return item;
}

Order parseJsonOrder(const QJsonValue &val)
{
    Order order = {};

    if (!val.isObject())
        return order;

    JsonObjectTracker object(val.toObject(), __func__);
    order.billing.address            = parseJsonAddress(object.value("billing_address"));
    order.billing.useShippingAddress = object.value("billing_address_same_as_shipping_address").toBool();

    order.id                         = object.value("id").toInt();

    order.currency                   = object.value("currency").toString();
    order.subtotal                   = object.value("subtotal").toDouble();
    order.taxableAmount              = object.value("taxable_amount").toDouble();
    order.total                      = object.value("total").toDouble();
    order.payout                     = object.value("payout").toDouble();
    order.lectronzFee                = object.value("lectronz_fee").toDouble();
    order.paymentFee                 = object.value("payment_fees").toDouble();

    const QJsonObject payment        = object.value("payment").toObject();
    order.payment.provider  = payment.value("provider").toString();
    order.payment.reference = payment.value("reference").toString();

    order.createdAt                  = QDateTime::fromString(object.value("created_at").toString(), Qt::ISODateWithMs).toLocalTime();
    order.updatedAt                  = QDateTime::fromString(object.value("updated_at").toString(), Qt::ISODateWithMs).toLocalTime();
    order.fulfilledAt                = QDateTime::fromString(object.value("fulfilled_at", true).toString(), Qt::ISODateWithMs).toLocalTime();

    order.status                     = object.value("status").toString();
    order.storeUrl                   = object.value("store_url").toString();
    order.customerEmail              = object.value("customer_email").toString();
    order.customerPhone              = object.value("customer_phone").toString();

    const QJsonArray items = object.value("items").toArray();
    for (const QJsonValue &item : items)
        order.items << parseJsonItem(item);

    order.tax.appliesToShipping      = object.value("tax_applies_to_shipping").toBool();
    order.tax.rate                   = object.value("tax_rate").toDouble();
    order.tax.total                  = object.value("total_tax").toDouble();
    order.tax.collected              = object.value("tax_collected").toDouble();

    order.shipping.address           = parseJsonAddress(object.value("shipping_address"));
    order.shipping.cost              = object.value("shipping_cost").toDouble();
    order.shipping.method            = object.value("shipping_method").toString();

    order.tracking.code              = object.value("tracking_code").toString();
    order.tracking.url               = object.value("tracking_url").toString();

    return order;
}

QString Order::editUrl() const
{
    return QString("https://lectronz.com/orders/%1/edit").arg(id);
}

QString Order::customerInvoiceUrl() const
{
    return QString("https://lectronz.com/orders/%1/customer_invoice").arg(id);
}

QString Order::supplierInvoiceUrl() const
{
    return QString("https://lectronz.com/orders/%1/supplier_invoice").arg(id);
}

void Order::openInBrowser() const
{
    QDesktopServices::openUrl(QUrl(editUrl()));
}

void Order::copyFullAddress() const
{
    QString address;
    address += shipping.address.firstName + " " + shipping.address.lastName + "\n";

    if (!shipping.address.organization.isEmpty())
        address += shipping.address.organization + "\n";

    address += shipping.address.street + "\n";

    if (!shipping.address.streetExtension.isEmpty())
        address += shipping.address.streetExtension + "\n";

    const QString state = (shipping.address.state.isEmpty() ? "" : ", " + shipping.address.state);
    address += shipping.address.city + state + " " + shipping.address.postalCode + "\n";
    address += shipping.address.country + "\n";

    if (!customerPhone.isEmpty())
        address += customerPhone + "\n";

    if (!customerEmail.isEmpty())
        address += customerEmail + "\n";

    if (address.endsWith("\n"))
        address = address.chopped(1);

    qApp->clipboard()->setText(address);
}
