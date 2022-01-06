#include "ordersortfiltermodel.h"

#include <QDateTime>

OrderSortFilterModel::OrderSortFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(-1);
}

bool OrderSortFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const int column = left.column();

    if (column == 0) // Id
        return left.data().toInt() < right.data().toInt();

    if ((column == 1) || (column == 8) || (column == 9)) // Created at, Updated at, Fulfilled on
        return left.data(Qt::UserRole + 1).toDateTime() < right.data(Qt::UserRole + 1).toDateTime();

    if (column == 2) // Total
        return left.data(Qt::UserRole + 1).toDouble() < right.data(Qt::UserRole + 1).toDouble();

    return QSortFilterProxyModel::lessThan(left, right);
}

bool OrderSortFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // column filters
    QHashIterator<int, QStringList> it(m_columnFilters);
    while (it.hasNext()) {
        it.next();

        const QStringList &filters = it.value();

        const QModelIndex index = sourceModel()->index(sourceRow, it.key(), sourceParent);
        const QString value = index.data().toString();

        if (!filters.contains(value))
            return false;
    }

    // date filters
    const QModelIndex createdAtIndex = sourceModel()->index(sourceRow, 1, sourceParent);
    const QDateTime createdAt = createdAtIndex.data(Qt::UserRole + 1).toDateTime();
    if ((createdAt < m_startDate) || (createdAt > m_endDate))
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void OrderSortFilterModel::setColumnFilters(const int column, const QStringList &filters)
{
    m_columnFilters.insert(column, filters);

    invalidate();
}

void OrderSortFilterModel::setDateFilter(const QDateTime &startDate, const QDateTime &endDate)
{
    m_startDate = startDate;
    m_endDate = endDate;

    invalidate();
}
