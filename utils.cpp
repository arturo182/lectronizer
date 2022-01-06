#include "utils.h"

// Use "Today" for today, "Yesterday" for yesterday, day name for the last 7 days,
// and a full date for older dates
QString friendlyDate(const QDateTime &date)
{
    const QDateTime now = QDateTime::currentDateTime();
    const int days = date.daysTo(now);
    const QString timeStr = date.time().toString("hh:mm");

    if (days == 0)
        return "Today, " + timeStr;

    if (days == 1)
        return "Yesterday, " + timeStr;

    if (days < 7)
        return date.toString("dddd, hh:mm");

    return date.toString("dd MMMM, hh:mm");
}
