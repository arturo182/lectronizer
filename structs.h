#pragma once

#include <QDateTime>
#include <QDebug>
#include <QJsonValue>
#include <QString>

struct Address
{
    QString firstName;
    QString lastName;
    QString organization;

    QString street;
    QString streetExtension;

    QString postalCode;
    QString city;
    QString state;
    QString country;
    QString countryCode;
};
QDebug operator<<(QDebug debug, const Address &a);

struct ItemOption
{
    QString sku;
    QString name;
    QString choice;
};
QDebug operator<<(QDebug debug, const ItemOption &op);

struct Item
{
    struct {
        int id;
        QString name;
        QString sku;
        QString description;
    } product;
    QList<ItemOption> options;
    int qty;
        double price;
};
QDebug operator<<(QDebug debug, const Item &it);

struct Order
{
    struct {
        Address address;
        bool useShippingAddress;
    } billing;

    int id;

    QString currency;
    double subtotal;
    double taxableAmount;
    double total;
    double payout;
    double lectronzFee;
    double paymentFee;

    struct {
        QString provider;
        QString reference;
    } payment;

    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime fulfilledAt;

    QString status;
    QString storeUrl;
    QString customerEmail;
    QString customerPhone;
    QList<Item> items;

    struct {
        bool appliesToShipping;
        double rate;
        double total;
        double collected;
    } tax;

    struct {
        Address address;
        double cost;
        QString method;
    } shipping;

    struct {
        QString code;
        QString url;
    } tracking;
};
QDebug operator<<(QDebug debug, const Order &o);

Order parseJsonOrder(const QJsonValue &val);
