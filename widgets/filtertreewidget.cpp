#include "filterbuttondelegate.h"
#include "filtertreewidget.h"

#include <QApplication>
#include <QSettings>
#include <QStandardItemModel>

static const int ColumnRole  = Qt::UserRole + 0;
static const int NameRole    = Qt::UserRole + 1;
static const int UseDataRole = Qt::UserRole + 2;

FilterTreeWidget::FilterTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setItemDelegate(new FilterButtonDelegate(this));

    connect(this, &QTreeWidget::itemPressed, this, &FilterTreeWidget::expandClickedItem);
    connect(this, &QTreeWidget::itemChanged, this, &FilterTreeWidget::processCheckBox);
}

void FilterTreeWidget::setOrderModel(QStandardItemModel *orderModel)
{
    m_orderModel = orderModel;
}

void FilterTreeWidget::addFilter(const ModelColumn column, bool useData)
{
    if (!m_orderModel) {
        qDebug() << "Model empty, can't add a filter!";
        return;
    }

    const QString name = m_orderModel->horizontalHeaderItem((int)column)->text();

    QTreeWidgetItem *root = new QTreeWidgetItem(this);
    root->setText(0, QString("%1 (0/0)").arg(name));
    root->setData(0, ColumnRole, (int)column);
    root->setData(0, NameRole, name); // for updating the text later
    root->setData(0, UseDataRole, useData);

    const auto addItem = [root](const QString &itemName)
    {
        QTreeWidgetItem *treeItem = new QTreeWidgetItem(root);
        treeItem->setText(0, itemName);
        treeItem->setCheckState(0, Qt::Checked);
    };

    addItem(tr("All"));

    root->setExpanded(true);
}

void FilterTreeWidget::setFilter(const QString &name, const QString &value)
{
    QTreeWidgetItem *topItem = nullptr;

    // find category
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = topLevelItem(i);
        if (item->data(0, NameRole).toString() != name)
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

void FilterTreeWidget::refreshFilters()
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *categoryItem = topLevelItem(i);
        const int column = categoryItem->data(0, ColumnRole).toInt();
        const bool useData = categoryItem->data(0, UseDataRole).toBool();

        QStringList values;

        // collect values
        const int rowCount = m_orderModel->rowCount();
        for (int row = 0; row < rowCount; ++row) {
            QStandardItem *item = m_orderModel->item(row, column);

            if (useData) {
                values << item->data().toStringList();
            } else {
                values << item->text();
            }
        }

        // sort and unique
        values.removeDuplicates();
        values.sort(Qt::CaseInsensitive);

        // add if new
        for (int j = 0; j < values.size(); ++j) {
            const QString value = values[j];

            bool found = false;
            for (int child = 0; child < categoryItem->childCount(); ++child) {
                if (categoryItem->child(child)->text(0) == value) {
                    found = true;
                    break;
                }
            }

            // already exists
            if (found)
                continue;

            QTreeWidgetItem *childItem = new QTreeWidgetItem();
            childItem->setText(0, value);

            // we can use the idx in values list because it reflects what the filter list will be after we're done
            // +1 cause "All" is 0
            categoryItem->insertChild(j + 1, childItem);

            // after insertChild() so processCheckBox() is called
            childItem->setCheckState(0, Qt::Checked);
        }
    }
}

void FilterTreeWidget::readSettings()
{
    QSettings set;

    set.beginGroup("FilterTreeWidget");

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *categoryItem = topLevelItem(i);
        const int column = categoryItem->data(0, ColumnRole).toInt();

        categoryItem->setExpanded(set.value(QString::number(column), true).toBool());
    }
}

void FilterTreeWidget::writeSettings() const
{
    QSettings set;

    set.beginGroup("FilterTreeWidget");

    for (int i = 0; i < topLevelItemCount(); ++i) {
        QTreeWidgetItem *categoryItem = topLevelItem(i);
        const int column = categoryItem->data(0, ColumnRole).toInt();

        set.setValue(QString::number(column), categoryItem->isExpanded());
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

    const QString columnName = parent->data(0, NameRole).toString();
    parent->setText(0, QString("%1 (%2/%3)").arg(columnName).arg(checkedCount).arg(parent->childCount() - 1));

    setProperty("processing", false);

    QStringList filters;

    // "All" is a special case
    if (allItem->checkState(0) == Qt::Checked)
        filters << "*";

    for (int i = 1; i < parent->childCount(); ++i) {
        QTreeWidgetItem *treeItem = parent->child(i);
        if (treeItem->checkState(0) == Qt::Checked)
            filters << treeItem->text(0);
    }

    const int modelColumn = parent->data(0, ColumnRole).toInt();
    emit filterChanged(modelColumn, filters, parent->data(0, UseDataRole).toBool());
}
