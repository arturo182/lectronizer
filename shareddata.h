#pragma once

#include <QHash>
#include <QStringList>

struct SharedData
{
    QString apiKey{};
    QHash<QString, qreal> currencyRates{};
    QString targetCurrency{"EUR"};
    bool showedTrayHint{};
    bool closeToSystemTray{};
    bool autoFetchWhenMinimized{};
    int autoFetchIntervalMin{};
    QString trackingUrl{};
};
