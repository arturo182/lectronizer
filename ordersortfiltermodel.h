#pragma once

#include <QDateTime>
#include <QSortFilterProxyModel>

class OrderSortFilterModel : public QSortFilterProxyModel
{
    public:
        struct ColumnFilter
        {
            QStringList filters{};
            bool useData{false};
        };

    public:
        OrderSortFilterModel(QObject *parent = nullptr);

        bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

        void setColumnFilters(const int column, const QStringList &filters, const bool useData);
        void setDateFilter(const QDateTime &startDate, const QDateTime &endDate);

    private:
        QHash<int, ColumnFilter> m_columnFilters;
        QDateTime m_startDate;
        QDateTime m_endDate;
};
