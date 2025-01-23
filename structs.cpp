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

    debug.nospace() << "ItemOption(sku = " << op.sku << ", name = " << op.name << ", choice = " << op.choice << ", weight = " << op.weight << ')';

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
                    << ", price = " << it.price
                    << ", weight = " << it.weight
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
    debug << ", storeId = " << o.storeId;
    debug << ", storeUrl = " << o.storeUrl;
    debug << ", customerLegalStatus = " << o.customerLegalStatus;
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
    debug << ", tracking = { code = " << o.tracking.code << ", url = " << o.tracking.url << ", required = " << o.tracking.required << "}";
    debug << ", weight = { base = " << o.weight.base << ", total = " << o.weight.total << ", unit = " << o.weight.unit << "}";

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
    itemOp.weight  = object.value("weight").toDouble();

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
    item.product.sku         = object.value("sku", true).toString();
    item.product.description = object.value("product_description").toString();
    item.qty                 = object.value("quantity").toInt();
    item.price               = object.value("price").toDouble();
    item.discount            = object.value("discount", true).toDouble();
    item.weight              = object.value("weight").toDouble();

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
    order.payment.provider           = payment.value("provider").toString();
    order.payment.reference          = payment.value("reference").toString();

    order.createdAt                  = QDateTime::fromString(object.value("created_at").toString(), Qt::ISODateWithMs).toLocalTime();
    order.updatedAt                  = QDateTime::fromString(object.value("updated_at").toString(), Qt::ISODateWithMs).toLocalTime();
    order.fulfilledAt                = QDateTime::fromString(object.value("fulfilled_at", true).toString(), Qt::ISODateWithMs).toLocalTime();
    order.fulfillUntil               = QDateTime::fromString(object.value("fulfill_until", true).toString(), Qt::ISODateWithMs).toLocalTime();

    order.status                     = object.value("status").toString();
    order.storeId                    = object.value("store_id").toInt();
    order.storeUrl                   = object.value("store_url").toString();
    order.customerLegalStatus        = object.value("customer_legal_status").toString();
    order.customerEmail              = object.value("customer_email").toString();
    order.customerPhone              = object.value("customer_phone").toString();
    order.customerNote               = object.value("customer_note", true).toString();

    const QJsonArray items = object.value("items").toArray();
    for (const QJsonValue &item : items)
        order.items << parseJsonItem(item);

    const QJsonArray codes = object.value("discount_codes", true).toArray();
    for (const QJsonValue &code : codes)
        order.discountCodes << code.toString();

    order.tax.appliesToShipping      = object.value("tax_applies_to_shipping").toBool();
    order.tax.rate                   = object.value("tax_rate").toDouble();
    order.tax.total                  = object.value("total_tax").toDouble();
    order.tax.collected              = object.value("tax_collected").toDouble();
    order.tax.number                 = object.value("customer_tax_id").toString();

    order.shipping.address           = parseJsonAddress(object.value("shipping_address"));
    order.shipping.cost              = object.value("shipping_cost").toDouble();
    order.shipping.method            = object.value("shipping_method").toString();

    order.tracking.required          = object.value("shipping_is_tracked").toBool();
    order.tracking.code              = object.value("tracking_code").toString();
    order.tracking.url               = object.value("tracking_url").toString();

    const QJsonObject weight         = object.value("shipping_weight").toObject();
    order.weight.base  = weight.value("base").toDouble();
    order.weight.total = weight.value("total").toDouble();
    order.weight.unit  = weight.value("weight_unit").toString();

    return order;
}

bool Order::isRefunded() const
{
    return status == "refunded";
}

bool Order::isShipped() const
{
    return !fulfilledAt.isNull();
}

bool Order::isPackaged() const
{
    return packaging >= 0;
}

QString Order::statusString() const
{
    if (isRefunded())
        return QObject::tr("Refunded");

    if (isShipped())
        return QObject::tr("Shipped");

    if (isPackaged())
        return QObject::tr("Packaged");

    if (status == "payment_success")
        return QObject::tr("Paid");

    return QObject::tr("Unknown");
}

QString Order::editUrl() const
{
    return QString("https://lectronz.com/seller/orders/%1/edit").arg(id);
}

QString Order::customerInvoiceUrl() const
{
    return QString("https://lectronz.com/seller/orders/%1/customer_invoice.pdf").arg(id);
}

QString Order::supplierInvoiceUrl() const
{
    return QString("https://lectronz.com/seller/orders/%1/supplier_invoice.pdf").arg(id);
}

QString Order::itemListing() const
{
    QStringList itemValues;
    for (int i = 0; i < items.count(); ++i) {
        const Item &item = items[i];

        QString value = QString("%1x %2").arg(item.qty).arg(item.product.name);
        if (item.options.size())
            value += QObject::tr(" (+options)");


        itemValues << value;
    }

    return itemValues.join(", ");
}

double Order::calcWeight() const
{
    double totalWeight = weight.base;

    for (const Item &item : items)
        totalWeight += item.qty * item.weight;

    return totalWeight;
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
