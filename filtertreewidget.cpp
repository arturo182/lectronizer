#include "filterbuttondelegate.h"
#include "filtertreewidget.h"

#include <QApplication>

FilterTreeWidget::FilterTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setItemDelegate(new FilterButtonDelegate(this));

    connect(this, &QTreeWidget::itemPressed, this, &FilterTreeWidget::expandClickedItem);
    connect(this, &QTreeWidget::itemChanged, this, &FilterTreeWidget::processCheckBox);
}

void FilterTreeWidget::addFilter(const int modelColumn, const QString &name, const QStringList &items)
{
    QTreeWidgetItem *root = new QTreeWidgetItem(this);
    root->setText(0, QString("%1 (0/0)").arg(name));
    root->setData(0, Qt::UserRole + 0, modelColumn);
    root->setData(0, Qt::UserRole + 1, name); // for updating the text later

    const auto addItem = [&, root](const QString &itemName)
    {
        QTreeWidgetItem *treeItem = new QTreeWidgetItem(root);
        treeItem->setText(0, itemName);
        treeItem->setCheckState(0, Qt::Checked);
    };

    addItem("All");
    for (const QString &item : items)
        addItem(item);

    root->setExpanded(true);
}

void FilterTreeWidget::setFilter(const QString &name, const QString &value)
{
    QTreeWidgetItem *topItem = nullptr;

    // find category
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = topLevelItem(i);
        if (item->data(0, Qt::UserRole + 1).toString() != name)
            continue;

        topItem = item;
        break;
    }

    if (!topItem || (topItem->childCount() == 0))
        return;

    // deselect "All"
    topItem->child(0)->setCheckState(0, Qt::Unchecked);

    // find value and select
    for (int i = 0; i < topItem->childCount(); ++i) {
        QTreeWidgetItem *child = topItem->child(i);

        if (child->text(0) != value)
            continue;

        child->setCheckState(0, Qt::Checked);
        break;
    }
}

void FilterTreeWidget::expandClickedItem(QTreeWidgetItem *item)
{
    if (!item)
        return;

    if (qApp->mouseButtons() != Qt::LeftButton)
        return;

    if (item->parent())
        return;

    item->setExpanded(!item->isExpanded());
}

void FilterTreeWidget::processCheckBox(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    QTreeWidgetItem *parent = item->parent();
    if (!parent) // top level item
        return;

    const bool isChecked = (item->checkState(0) == Qt::Checked);
    QTreeWidgetItem *allItem = parent->child(0);
    if (!allItem)
        return;

    // use a flag so we don't recurse
    if (property("processing").toBool())
        return;

    setProperty("processing", true);

    int checkedCount = 0;

    if (item == allItem) {
        for (int i = 1; i < parent->childCount(); ++i) {
            QTreeWidgetItem *otherItem = parent->child(i);
            if (item->checkState(0) != Qt::PartiallyChecked) {
                otherItem->setCheckState(0, isChecked ? Qt::Checked : Qt::Unchecked);

                checkedCount += isChecked;
            }
        }
    } else {
        bool allChecked = true;
        bool allUnchecked = true;
        for (int i = 1; i < parent->childCount(); ++i) {
            QTreeWidgetItem *otherItem = parent->child(i);
            if (otherItem->checkState(0) == Qt::Checked) {
                allUnchecked = false;

                checkedCount += 1;
            } else {
                allChecked = false;
            }
        }

        if (allChecked) {
            allItem->setCheckState(0, Qt::Checked);
        } else if (allUnchecked) {
            allItem->setCheckState(0, Qt::Unchecked);
        } else {
            allItem->setCheckState(0, Qt::PartiallyChecked);
        }
    }

    const QString columnName = parent->data(0, Qt::UserRole + 1).toString();
    parent->setText(0, QString("%1 (%2/%3)").arg(columnName).arg(checkedCount).arg(parent->childCount() - 1));

    setProperty("processing", false);

    QStringList filters;

    // we can skip "All" because it sets all to checked anyway
    for (int i = 1; i < parent->childCount(); ++i) {
        QTreeWidgetItem *treeItem = parent->child(i);
        if (treeItem->checkState(0) == Qt::Checked)
            filters << treeItem->text(0);
    }

    const int modelColumn = parent->data(0, Qt::UserRole).toInt();
    emit filterChanged(modelColumn, filters);
}
