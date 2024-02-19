#pragma once

#include <QDateTime>

#include "shareddata.h"

QString textDate(const QDateTime &date, const SharedData &shared);
QString sanitizePhoneNumber(const QString &phone, const QString &country, const SharedData &shared);
