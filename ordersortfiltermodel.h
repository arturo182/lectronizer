#pragma once

#include <QDateTime>
#include <QSortFilterProxyModel>

class OrderSortFilterModel : public QSortFilterProxyModel
{
    public:
        OrderSortFilterModel(QObject *parent = nullptr);

        bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

        void setColumnFilters(const int column, const QStringList &filters);
        void setDateFilter(const QDateTime &startDate, const QDateTime &endDate);

    private:
        QHash<int, QStringList> m_columnFilters;
        QDateTime m_startDate;
        QDateTime m_endDate;
};
