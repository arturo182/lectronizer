#pragma once

#include "enums.h"
#include "structs.h"

#include <QTreeWidget>

class QStandardItemModel;

class FilterTreeWidget : public QTreeWidget
{
    Q_OBJECT

    public:
        explicit FilterTreeWidget(QWidget *parent = nullptr);

        void setOrderModel(QStandardItemModel *orderModel);

        void addFilter(const ModelColumn column);
        void setFilter(const QString &name, const QString &value);

        void refreshFilters();

    private:
        void expandClickedItem(QTreeWidgetItem *item);
        void processCheckBox(QTreeWidgetItem *item, int column);

    signals:
        void filterChanged(const int column, const QStringList &filters);

    private:
        QStandardItemModel *m_orderModel{};
};
