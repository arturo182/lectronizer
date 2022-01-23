#pragma once

#include <QDateTime>
#include <QDebug>
#include <QJsonValue>
#include <QString>

struct Address
{
    QString firstName{};
    QString lastName{};
    QString organization{};

    QString street{};
    QString streetExtension{};

    QString postalCode{};
    QString city{};
    QString state{};
    QString country{};
    QString countryCode{};

    bool operator==(const Address &other) const
    {
        return std::tie(firstName, lastName, organization, street, streetExtension,
                        postalCode, city, state, country, countryCode) ==
                    std::tie(other.firstName, other.lastName, other.organization, other.street, other.streetExtension,
                             other.postalCode, other.city, other.state, other.country, other.countryCode);
    }
};
QDebug operator<<(QDebug debug, const Address &a);

struct ItemOption
{
    QString sku{};
    QString name{};
    QString choice{};
    double weight{};

    bool operator==(const ItemOption &other) const
    {
        return std::tie(sku, name, choice, weight) ==
                    std::tie(other.sku, other.name, other.choice, other.weight);
    }
};
QDebug operator<<(QDebug debug, const ItemOption &op);

struct Item
{
    struct Product
    {
        int id{};
        QString name{};
        QString sku{};
        QString description{};

        bool operator==(const Product &other) const
        {

            return std::tie(id, name, sku, description)
                    == std::tie(other.id, other.name, other.sku, other.description);
        }
    } product{};
    QList<ItemOption> options{};
    int qty{};
    double price{};
    double weight{};

    bool packaged{}; // non-api

    bool operator==(const Item &other) const
    {
        return std::tie(product, options, qty, price, weight, packaged) ==
                    std::tie(other.product, other.options, other.qty, other.price, other.weight, other.packaged);
    }
};
QDebug operator<<(QDebug debug, const Item &it);

enum class OrderStatus
{
    Unknown = 0,
    Paid,
    Packaged,
    Shipped,
    // TODO: Delivered,
};

struct Order
{
    struct Billing
    {
        Address address{};
        bool useShippingAddress{};

        bool operator==(const Billing &other) const
        {
            return std::tie(address, useShippingAddress) ==
                        std::tie(other.address, other.useShippingAddress);
        }
    } billing{};

    int id{};

    QString currency{};
    double subtotal{};
    double taxableAmount{};
    double total{};
    double payout{};
    double lectronzFee{};
    double paymentFee{};

    struct Payment
    {
        QString provider{};
        QString reference{};

        bool operator==(const Payment &other) const
        {
            return std::tie(provider, reference) ==
                        std::tie(other.provider, other.reference);
        }
    } payment{};

    QDateTime createdAt{};
    QDateTime updatedAt{};
    QDateTime fulfilledAt{};

    QString status{};
    QString storeUrl{};
    QString customerEmail{};
    QString customerPhone{};
    QList<Item> items{};

    struct Tax
    {
        bool appliesToShipping{};
        double rate{};
        double total{};
        double collected{};

        bool operator==(const Tax &other) const
        {
            return std::tie(appliesToShipping, rate, total, collected) ==
                        std::tie(other.appliesToShipping, other.rate, other.total, other.collected);
        }
    } tax{};

    struct Shipping
    {
        Address address{};
        double cost{};
        QString method{};

        bool operator==(const Shipping &other) const
        {
            return std::tie(address, cost, method) ==
                        std::tie(other.address, other.cost, other.method);
        }
    } shipping{};

    struct Tracking
    {
        bool required{};
        QString code{};
        QString url{};

        bool operator==(const Tracking &other) const
        {
            return std::tie(required, code, url) ==
                        std::tie(other.required, other.code, other.url);
        }
    } tracking{};

    struct Weight
    {
        QString unit{};
        double total{};
        double base{};

        bool operator==(const Weight &other) const
        {
            return std::tie(unit, total, base) ==
                        std::tie(other.unit, other.total, other.base);
        }
    } weight{};

    int packaging{-1}; // non-api

    QString statusString() const;

    QString editUrl() const;
    QString customerInvoiceUrl() const;
    QString supplierInvoiceUrl() const;
    QString itemListing() const;

    double calcWeight() const;

    void openInBrowser() const;
    void copyFullAddress() const;

    bool operator==(const Order &other) const
    {
        return std::tie(billing, id, currency, subtotal, taxableAmount, total, payout, lectronzFee,
                        paymentFee, payment, createdAt, updatedAt, fulfilledAt, status, storeUrl,
                        customerEmail, customerPhone, items, tax, shipping, tracking, weight, packaging) ==
                    std::tie(other.billing, other.id, other.currency, other.subtotal, other.taxableAmount,
                             other.total, other.payout, other.lectronzFee, other. paymentFee, other.payment,
                             other.createdAt, other.updatedAt, other.fulfilledAt, other.status, other.storeUrl,
                             other.customerEmail, other.customerPhone, other.items, other.tax, other.shipping,
                             other.tracking, other.weight, other.packaging);
    }
    bool operator!=(const Order &other) const { return !(*this == other); };
};
QDebug operator<<(QDebug debug, const Order &o);

Order parseJsonOrder(const QJsonValue &val);

struct Packaging
{
    int id{};
    QString name{};
    int stock{};
    QString restockUrl{};
};
