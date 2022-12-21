#pragma once

#include <QDateTime>

#include "shareddata.h"

QString textDate(const QDateTime &date, bool friendly);
QString sanitizePhoneNumber(const QString &phone, const QString &country, const SharedData &shared);
