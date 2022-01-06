#pragma once

#include <QTreeWidget>

class FilterTreeWidget : public QTreeWidget
{
    Q_OBJECT

    public:
        explicit FilterTreeWidget(QWidget *parent = nullptr);

        void addFilter(const int modelColumn, const QString &name, const QStringList &items);

    private:
        void expandClickedItem(QTreeWidgetItem *item);
        void processCheckBox(QTreeWidgetItem *item, int column);

    signals:
        void filterChanged(const int column, const QStringList &filters);
};

