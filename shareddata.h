#pragma once

#include "widgets/bulkexporterdialog.h"

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
    QString dateFormat{};
    int autoFetchIntervalMin{};
    QString trackingUrl{};
    int csvSeparator{BulkExporterDialog::SepComma};

    // Phone number sanitization
    bool phoneRemoveDashes{};
    bool phoneRemoveSpaces{};
    bool phoneAddCountryCode{};
    bool phoneUsePlusPrefix{};
};
