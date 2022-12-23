#pragma once

#include "enums.h"

#include <QTreeWidget>

class QStandardItemModel;

class FilterTreeWidget : public QTreeWidget
{
    Q_OBJECT

    public:
        explicit FilterTreeWidget(QWidget *parent = nullptr);

        void setOrderModel(QStandardItemModel *orderModel);

        void addFilter(const ModelColumn column, bool useData = false);
        void setFilter(const QString &name, const QString &value);
        void setFilters(const QString &name, const QStringList &values);

        void refreshFilters();

        void readSettings();
        void writeSettings() const;

    private:
        void expandClickedItem(QTreeWidgetItem *item);
        void processCheckBox(QTreeWidgetItem *item, int column);

    signals:
        void filterChanged(const int column, const QStringList &filters, const bool useData);

    private:
        QStandardItemModel *m_orderModel{};
};
