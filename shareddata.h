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
    bool friendlyDate{};
    int autoFetchIntervalMin{};
    QString trackingUrl{};

    // Phone number sanitization
    bool phoneRemoveDashes{};
    bool phoneRemoveSpaces{};
    bool phoneAddCountryCode{};
    bool phoneUsePlusPrefix{};
};
