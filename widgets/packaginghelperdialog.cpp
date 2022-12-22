#include "orderitemdelegate.h"
#include "ordermanager.h"
#include "packaginghelperdialog.h"
#include "sqlmanager.h"
#include "ui_packaginghelperdialog.h"

#include <QFontMetrics>
#include <QItemDelegate>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextLayout>
#include <QTextOption>

enum MatchLogic
{
    Required = 0,
    Optional,
    Exclude,
};

enum OrderSort
{
    Id = 0,
    Count,
    Weight,
    Shipping,
    Total,
};

class OrderListDelegate : public QItemDelegate
{
    public:
        explicit OrderListDelegate(OrderManager *orderMgr, QObject *parent = nullptr)
            : QItemDelegate{parent}
            , m_orderMgr{orderMgr}
        {

        }

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            const QString text = index.data().toString();
            const int id = index.data(Qt::UserRole).toInt();
            const Order &order = m_orderMgr->order(id);

            const int margin = option.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, option.widget) + 1;

            QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
            if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
                cg = QPalette::Inactive;

            painter->save();

            // draw background
            drawBackground(painter, option, index);

            // draw text
            if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
                painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
            } else {
                painter->setPen(option.palette.color(cg, QPalette::Text));
            }

            const QRect rect(option.rect.x() + margin, option.rect.y(), option.rect.width() - margin * 2, option.rect.height() / 2);
            QRect boundingRect;
            painter->drawText(rect, Qt::AlignBottom, text, &boundingRect);

            // draw tick
            if (order.isPackaged()) {
                const QPixmap tick = QPixmap(":/res/icons/tick.png").scaled(boundingRect.height() - 2, boundingRect.height() - 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                painter->drawPixmap(QPoint(boundingRect.right() + margin + 1, boundingRect.top() + 1), tick);
            }

            // draw sub text
            const QString subText = tr("items: %1 | %2 | %3 | %4")
                                        .arg(std::accumulate(order.items.begin() + 1,
                                                             order.items.end(),
                                                             QString::number(order.items[0].qty),
                                                             [](const QString &qty, const Item &item) { return qty + "+" + QString::number(item.qty); }))
                                        .arg(QString("%1%2").arg(order.calcWeight(), 0, 'f', 1).arg(order.weight.unit))
                                        .arg(QString("%1 %2").arg(order.total, 0, 'f', 2).arg(order.currency))
                                        .arg(order.shipping.method);

            QFont subFont = option.font;
            subFont.setPointSize(subFont.pointSize() - 1);
            QFontMetrics subFm(subFont);
            painter->setFont(subFont);
            if (!(option.showDecorationSelected && (option.state & QStyle::State_Selected)))
                painter->setPen(option.palette.color(cg, QPalette::Shadow));

            const QRect subRect(option.rect.x() + margin * 3, option.rect.y() + option.rect.height() / 2, option.rect.width() - margin * 4, option.rect.height() / 2);
            painter->drawText(subRect, Qt::AlignTop, subFm.elidedText(subText, Qt::ElideRight, subRect.width()));

            painter->restore();
        }

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
        {
            const QFontMetrics &fm = option.fontMetrics;

            QFont subFont = option.font;
            subFont.setPointSize(subFont.pointSize() - 1);
            const QFontMetrics subFm = QFontMetrics(subFont);

            return QSize(fm.horizontalAdvance(index.data().toString()), 1 + fm.height() + subFm.height() + 1);
        }

    private:
        OrderManager *m_orderMgr{};
};

static bool treeItemCmp(OrderManager *orderMgr, const OrderSort &sort, const QTreeWidgetItem *left, const QTreeWidgetItem *right)
{
    const Order &leftOrder = orderMgr->order(left->data(0, Qt::UserRole).toInt());
    const Order &rightOrder = orderMgr->order(right->data(0, Qt::UserRole).toInt());

    if (sort == OrderSort::Id)
        return leftOrder.id < rightOrder.id;

    if (sort == OrderSort::Count) {
        const auto itemCount = [](const Order &order)
        {
            int count = 0;
            for (const Item &item : order.items)
                count += item.qty;

            return count;
        };

        return itemCount(leftOrder) < itemCount(rightOrder);
    }

    if (sort == OrderSort::Weight)
        return leftOrder.calcWeight() < rightOrder.calcWeight();

    if (sort == OrderSort::Shipping)
        return leftOrder.shipping.method < rightOrder.shipping.method;

    return leftOrder.total < rightOrder.total;
};

PackagingHelperDialog::PackagingHelperDialog(OrderManager *orderMgr, SqlManager *sqlMgr, QWidget *parent)
    : QDialog{parent}
    , m_orderMgr{orderMgr}
    , m_sqlMgr{sqlMgr}
    , m_ui{new Ui::PackagingHelperDialog}
{
    m_ui->setupUi(this);
    m_ui->packagingSplitter->setStretchFactor(0, 1);
    m_ui->packagingSplitter->setStretchFactor(1, 2);
    m_ui->orderListTree->setItemDelegate(new OrderListDelegate(m_orderMgr, this));
    m_ui->orderItemTree->setItemDelegateForColumn(0, new OrderItemDelegate(m_orderMgr, this));

    m_ui->productSelectionTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_ui->productSelectionTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    m_ui->orderItemTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    m_ui->packagingComboBox->addItem(tr("Not packaged"), -1);
    m_ui->packagingComboBox->addItem(tr("Default packaging"), 0);
    for (const Packaging &pack : m_sqlMgr->packagings())
        m_ui->packagingComboBox->addItem(tr("%1 (%2 left)").arg(pack.name).arg(pack.stock), pack.id);

    reset();

    // shipping method filter change
    connect(m_ui->shippingSelectionTree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column)
    {
        processSelectionTreeCheckBox(m_ui->shippingSelectionTree->invisibleRootItem(), item, column);
    });

    connect(m_ui->startOverButton, &QPushButton::clicked, this, [this]()
    {
        if (m_ui->stackedWidget->currentWidget() == m_ui->welcomePage)
            return;

        reset();
    });

    connect(m_ui->pushButton, &QPushButton::clicked, this, [this]()
    {
        if (m_ui->stackedWidget->currentWidget() == m_ui->welcomePage) {
            fillOrderList();

            m_ui->stackedWidget->setCurrentIndex(1);

            m_ui->pushButton->setText(tr("< &Back"));
            m_ui->pushButton2->setText(tr("&Next >"));
            m_ui->startOverButton->setVisible(true);

            //select first item
            m_ui->orderListTree->setCurrentItem(m_ui->orderListTree->topLevelItem(0));
            m_ui->orderListTree->setFocus();
        } else if (m_ui->stackedWidget->currentWidget() == m_ui->packagingPage) {
            const int idx = m_ui->orderListTree->indexOfTopLevelItem(m_ui->orderListTree->currentItem());

            if (idx > 0)
                m_ui->orderListTree->setCurrentItem(m_ui->orderListTree->topLevelItem(idx - 1));
        }
    });
    connect(m_ui->pushButton2, &QPushButton::clicked, this, [this]()
    {
        if (m_ui->stackedWidget->currentWidget() == m_ui->welcomePage) {
            close();
        } else if (m_ui->stackedWidget->currentWidget() == m_ui->packagingPage) {
            const int idx = m_ui->orderListTree->indexOfTopLevelItem(m_ui->orderListTree->currentItem());

            if (idx < m_ui->orderListTree->topLevelItemCount() - 1)
                m_ui->orderListTree->setCurrentItem(m_ui->orderListTree->topLevelItem(idx + 1));
        }
    });

    // show order details
    connect(m_ui->orderListTree, &QTreeWidget::currentItemChanged, this, &PackagingHelperDialog::showOrderDetails);

    // change order list sorting
    connect(m_ui->orderListSortComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &PackagingHelperDialog::sortOrderList);

    // order item checked, update the order
    connect(m_ui->orderItemTree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item)
    {
        const QVariant orderIdVar = item->data(0, Qt::UserRole + 0);
        const QVariant itemIdxVar = item->data(0, Qt::UserRole + 1);

        if (!orderIdVar.isValid() || !itemIdxVar.isValid())
            return;

        const bool isPackaged = (item->checkState(0) == Qt::Checked);

        const int orderId = orderIdVar.toInt();
        const int itemIdx = itemIdxVar.toInt();

        Order &order = m_orderMgr->order(orderId);
        if (order.items[itemIdx].packaged != isPackaged) {
            order.items[itemIdx].packaged = isPackaged;
            emit m_orderMgr->orderUpdated(order);
        }
    });

    // packaging changed, mark packaged and update order and packaging status
    connect(m_ui->packagingComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]()
    {
        QTreeWidgetItem *orderItem = m_ui->orderListTree->currentItem();
        if (!orderItem)
            return;

        const QVariant orderIdVar = orderItem->data(0, Qt::UserRole + 0);
        if (!orderIdVar.isValid())
            return;

        const int packId = m_ui->packagingComboBox->currentData().toInt();

        const int orderId = orderIdVar.toInt();
        Order &order = m_orderMgr->order(orderId);

        const int prevPackId = order.packaging;
        if (packId == prevPackId)
            return;

        // check all items if packaged
        if (packId >= 0) {
            for (int i = 0; i < m_ui->orderItemTree->topLevelItemCount(); ++i) {
                QTreeWidgetItem *item = m_ui->orderItemTree->topLevelItem(i);
                item->setCheckState(0, Qt::Checked);
            }
        }

        m_orderMgr->setPackaging(orderId, packId);
        m_ui->orderListTree->viewport()->repaint();

        updateComboLabels();
    });

    readSettings();
}

PackagingHelperDialog::~PackagingHelperDialog()
{
    delete m_ui;
    m_ui = nullptr;
}

void PackagingHelperDialog::reset()
{
    m_ui->stackedWidget->setCurrentWidget(m_ui->welcomePage);

    m_ui->startOverButton->setVisible(false);

    m_ui->pushButton->setText(tr("&Start"));
    m_ui->pushButton->setEnabled(true);

    m_ui->pushButton2->setText(tr("&Close"));
    m_ui->pushButton2->setEnabled(true);

    m_ui->stackedWidget->setCurrentIndex(0);

    m_ui->shippingSelectionTree->clear();
    fillShippingTree();

    m_ui->productSelectionTree->clear();
    fillProductTree();

    m_ui->orderListTree->clear();

    m_ui->packagingComboBox->setCurrentIndex(0);

    updateMatchingOrderCount();
}

void PackagingHelperDialog::closeEvent(QCloseEvent *event)
{
    writeSettings();

    QWidget::closeEvent(event);
}

void PackagingHelperDialog::readSettings()
{
    QSettings set;

    set.beginGroup("PackagingHelper");
    restoreGeometry(set.value("geometry").toByteArray());
    setWindowState(Qt::WindowStates(set.value("state").toInt()));
    m_ui->packagingSplitter->restoreState(set.value("splitter").toByteArray());
    m_ui->orderListSortComboBox->setCurrentIndex(set.value("sort").toInt());
}

void PackagingHelperDialog::writeSettings() const
{
    QSettings set;

    set.beginGroup("PackagingHelper");
    set.setValue("geometry", saveGeometry());
    set.setValue("state", (int)windowState());
    set.setValue("splitter", m_ui->packagingSplitter->saveState());
    set.setValue("sort", m_ui->orderListSortComboBox->currentIndex());
}

void PackagingHelperDialog::fillShippingTree()
{
    const auto addItem = [this](const QString &itemName)
    {
        QTreeWidgetItem *treeItem = new QTreeWidgetItem(m_ui->shippingSelectionTree);
        treeItem->setText(0, itemName);
        treeItem->setCheckState(0, Qt::Checked);
    };

    // get all possible values from existing orders
    QStringList methods;

    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);
        if (order.isShipped() || order.isPackaged() || order.isRefunded())
            continue;

        methods << order.shipping.method;
    }

    // cleanup
    methods.removeDuplicates();
    methods.sort(Qt::CaseInsensitive);

    // fill tree
    addItem(tr("All"));
    for (const QString &method : methods)
        addItem(method);
}

void PackagingHelperDialog::fillProductTree()
{
    typedef QMap<QString, QStringList> ProductOptions; // option name -> all possible values

    QMap<QString, ProductOptions> products; // product name -> all options and their values

    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);

        // skip fulfilled and packaged orders
        if (order.isShipped() || order.isPackaged() || order.isRefunded())
            continue;

        for (const Item &item : order.items) {
            const QString &productName = item.product.name;
            if (!products.contains(productName))
                products.insert(productName, {});

            for (const ItemOption &option : item.options) {
                const QString &optionName = option.name;
                if (!products[productName].contains(optionName))
                    products[productName].insert(optionName, {});

                products[productName][optionName] << option.choice;
            }
        }
    }

    // cleanup
    for (auto &product : products) {
        for (auto &option : product) {
            option.removeDuplicates();
            option.sort(Qt::CaseInsensitive);
        }
    }

    QTreeWidget *tree = m_ui->productSelectionTree;

    // fill tree
    for (auto pit = products.begin(); pit != products.end(); ++pit) {
        const QString &productName = pit.key();

        QTreeWidgetItem *productItem = new QTreeWidgetItem(tree, { productName });

        // product logic
        QComboBox *productComboBox = new QComboBox(tree);
        productComboBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        productComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        // order must match MatchLogic enum
        productComboBox->addItem(QIcon(":/res/icons/exclamation_green.png"), tr("Must include"));
        productComboBox->addItem(QIcon(":/res/icons/question.png"), tr("May include"));
        productComboBox->addItem(QIcon(":/res/icons/cross.png"), tr("Must NOT include"));
        productComboBox->setCurrentIndex(1);
        connect(productComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, tree, productItem, productComboBox]()
        {
            const bool isNot = (productComboBox->currentIndex() == MatchLogic::Exclude);

            tree->itemWidget(productItem, 2)->setEnabled(!isNot);
            tree->itemWidget(productItem, 3)->setEnabled(!isNot);

            for (int i = 0; i < productItem->childCount(); ++i) {
                QTreeWidgetItem *childItem = productItem->child(i);
                tree->itemWidget(childItem, 1)->setEnabled(!isNot);
            }

            updateMatchingOrderCount();
        });
        tree->setItemWidget(productItem, 1, productComboBox);

        // product min qty
        QSpinBox *minQtySpinBox = new QSpinBox(tree);
        minQtySpinBox->setRange(1, INT_MAX);
        tree->setItemWidget(productItem, 2, minQtySpinBox);

        // product max qty
        QSpinBox *maxQtySpinBox = new QSpinBox(tree);
        maxQtySpinBox->setRange(1, INT_MAX);
        maxQtySpinBox->setValue(9999);
        tree->setItemWidget(productItem, 3, maxQtySpinBox);

        connect(minQtySpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this, minQtySpinBox, maxQtySpinBox]()
        {
            // make sure max is never less than min
            maxQtySpinBox->setValue(qMax(minQtySpinBox->value(), maxQtySpinBox->value()));

            updateMatchingOrderCount();
        });

        connect(maxQtySpinBox, qOverload<int>(&QSpinBox::valueChanged), this, [this, minQtySpinBox, maxQtySpinBox]()
        {
            // make sure min is never more than max
            minQtySpinBox->setValue(qMin(minQtySpinBox->value(), maxQtySpinBox->value()));

            updateMatchingOrderCount();
        });

        // options
        for (auto oit = pit.value().begin(); oit != pit.value().end(); ++oit) {
            const QString &optionName = oit.key();

            QTreeWidgetItem *optionItem = new QTreeWidgetItem(productItem, { optionName });

            QComboBox *optionComboBox = new QComboBox(m_ui->productSelectionTree);
            optionComboBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
            optionComboBox->addItem(tr("Any value"));
            optionComboBox->addItems(products[productName][optionName]);
            connect(optionComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &PackagingHelperDialog::updateMatchingOrderCount);

            m_ui->productSelectionTree->setItemWidget(optionItem, 1, optionComboBox);
        }
    }
}

void PackagingHelperDialog::fillOrderList()
{
    QList<QTreeWidgetItem*> items;
    for (const int id : filteredOrders()) {
        QTreeWidgetItem *orderItem = new QTreeWidgetItem();
        orderItem->setText(0, tr("Order #%1").arg(id));
        orderItem->setData(0, Qt::UserRole, id);
        items << orderItem;
    }

    std::sort(items.begin(), items.end(), [this](QTreeWidgetItem *left, QTreeWidgetItem *right)
    {
        return treeItemCmp(m_orderMgr, (OrderSort)m_ui->orderListSortComboBox->currentIndex(), left, right);
    });

    m_ui->orderListTree->addTopLevelItems(items);
}

void PackagingHelperDialog::processSelectionTreeCheckBox(QTreeWidgetItem *parent, QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    QTreeWidget *tree = parent->treeWidget();

    const bool isChecked = (item->checkState(0) == Qt::Checked);
    QTreeWidgetItem *allItem = parent->child(0);
    if (!allItem)
        return;

    // use a flag so we don't recurse
    if (tree->property("processing").toBool())
        return;

    tree->setProperty("processing", true);

    if (item == allItem) {
        for (int i = 1; i < parent->childCount(); ++i) {
            QTreeWidgetItem *otherItem = parent->child(i);

            if (item->checkState(0) != Qt::PartiallyChecked)
                otherItem->setCheckState(0, isChecked ? Qt::Checked : Qt::Unchecked);
        }
    } else {
        bool allChecked = true;
        bool allUnchecked = true;

        for (int i = 1; i < parent->childCount(); ++i) {
            QTreeWidgetItem *otherItem = parent->child(i);

            if (otherItem->checkState(0) == Qt::Checked) {
                allUnchecked = false;
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

    tree->setProperty("processing", false);

    updateMatchingOrderCount();
}

void PackagingHelperDialog::showOrderDetails(QTreeWidgetItem *current)
{
    if (!current) {
        m_ui->pushButton->setEnabled(false);
        m_ui->pushButton2->setEnabled(false);
        return;
    }

    const int id = current->data(0, Qt::UserRole).toInt();
    const Order &order = m_orderMgr->order(id);

    // labels
    m_ui->orderNameValueLabel->setText(QString("%1 %2").arg(order.shipping.address.firstName, order.shipping.address.lastName));
    m_ui->orderCountryValueLabel->setText(order.shipping.address.country);
    m_ui->orderWeightValueLabel->setText(QString("%1 %2").arg(order.calcWeight(), 0, 'f', 1).arg(order.weight.unit));
    m_ui->orderTrackingValueLabel->setText(order.tracking.required ? tr("Required") : tr("Not required"));
    m_ui->orderShippingValueLabel->setText(order.shipping.method);
    m_ui->orderTotalValueLabel->setText(QString("%1 %2").arg(order.total, 0, 'f', 2).arg(order.currency));
    m_ui->customerNoteTextEdit->setPlainText(order.customerNote);

    // item tree
    QFont boldFont = font();
    boldFont.setBold(true);

    m_ui->orderItemTree->clear();
    for (int i = 0; i < order.items.size(); ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->orderItemTree);
        item->setCheckState(0, order.items[i].packaged ? Qt::Checked : Qt::Unchecked);
        item->setData(0, Qt::UserRole + 0, order.id);
        item->setData(0, Qt::UserRole + 1, i);
    }

    // packaging combo labels
    updateComboLabels();

    // packaging combo index
    if (order.packaging < 0) {
        m_ui->packagingComboBox->setCurrentIndex(0);
    } else {
        const int idx = idxFromPackagingId(order.packaging);
        if (idx < 0) {
            qDebug() << "packaging" << order.packaging << "not found in combo box";
        } else {
            m_ui->packagingComboBox->setCurrentIndex(idx);
        }
    }

    // buttons
    updateOrderButtons();
}

void PackagingHelperDialog::updateMatchingOrderCount()
{
    const QList<int> orders = filteredOrders();

    m_ui->matchingOrdersLabel->setText(tr("%1 order(s) match the selected filters", "", orders.size()).arg(orders.size()));

    m_ui->pushButton->setEnabled(orders.size() > 0);
}

void PackagingHelperDialog::updateOrderButtons()
{
    if (m_ui->stackedWidget->currentWidget() != m_ui->packagingPage)
        return;

    const int idx = m_ui->orderListTree->indexOfTopLevelItem(m_ui->orderListTree->currentItem());

    m_ui->pushButton->setEnabled(idx > 0);
    m_ui->pushButton2->setEnabled(idx < m_ui->orderListTree->topLevelItemCount() - 1);
}

void PackagingHelperDialog::updateComboLabels()
{
    for (const Packaging &pack : m_sqlMgr->packagings()) {
        const int idx = idxFromPackagingId(pack.id);
        if (idx == -1)
            continue;

        m_ui->packagingComboBox->setItemText(idx, tr("%1 (%2 left)").arg(pack.name).arg(pack.stock));
    }
}

void PackagingHelperDialog::sortOrderList(const int idx)
{
    const OrderSort sort = (OrderSort)idx;

    QTreeWidget *tree = m_ui->orderListTree;
    QTreeWidgetItem *topItem = tree->invisibleRootItem();

    // bubble sort, lol
    for (int i = 0; i < topItem->childCount() - 1; ++i) {
        bool swapped = false;

        for (int j = 0; j < topItem->childCount() - i - 1; ++j) {
            if (!treeItemCmp(m_orderMgr, sort, topItem->child(j), topItem->child(j + 1))) {
                QTreeWidgetItem *item = topItem->takeChild(j);
                topItem->insertChild(j + 1, item);

                swapped = true;
            }
        }

        if (!swapped)
            break;
    }

    updateOrderButtons();
}

int PackagingHelperDialog::idxFromPackagingId(const int id)
{
    for (int i = 0; i < m_ui->packagingComboBox->count(); ++i) {
        const int packId = m_ui->packagingComboBox->itemData(i).toInt();
        if (packId == id)
            return i;
    }

    return -1;
}

QList<int> PackagingHelperDialog::filteredOrders() const
{
    QTreeWidget *tree = nullptr;

    // shipping
    QStringList shippingMethods;
    bool anyShipping = false;
    tree = m_ui->shippingSelectionTree;

    // If "All" is checked, no need to even check the rest
    if (tree->topLevelItem(0)->checkState(0) == Qt::Checked) {
        anyShipping = true;
    } else {
        for (int i = 1; i < tree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *treeItem = tree->topLevelItem(i);

            if (treeItem->checkState(0) == Qt::Checked)
                shippingMethods << treeItem->text(0);
        }
    }

    // products
    tree = m_ui->productSelectionTree;

    struct ProductFilter
    {
        QMap<QString, QString> options{};
        MatchLogic logic{};
        QPair<int, int> qtyRange{};
        bool matched{}; // used by algorithm later for all Required products
    };

    QHash<QString, ProductFilter> productFilters;

    // convert user selection to a list of ProductFilter objects
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *productItem = tree->topLevelItem(i);

        // get widgets
        QComboBox *productComboBox = qobject_cast<QComboBox*>(tree->itemWidget(productItem, 1));
        QSpinBox *minQtySpinBox = qobject_cast<QSpinBox*>(tree->itemWidget(productItem, 2));
        QSpinBox *maxQtySpinBox = qobject_cast<QSpinBox*>(tree->itemWidget(productItem, 3));

        // if the logic is "NOT", skip
        if (productComboBox->currentIndex() == MatchLogic::Exclude)
            continue;

        // create the filter
        ProductFilter filter{};
        filter.logic = (MatchLogic)productComboBox->currentIndex();
        filter.qtyRange = qMakePair(minQtySpinBox->value(), maxQtySpinBox->value());

        for (int j = 0; j < productItem->childCount(); ++j) {
            QTreeWidgetItem *optionItem = productItem->child(j);
            QComboBox *optionComboBox = qobject_cast<QComboBox*>(tree->itemWidget(optionItem, 1));

            // "Any value"
            if (optionComboBox->currentIndex() == 0) {
                filter.options.insert(optionItem->text(0), "*");
            } else {
                filter.options.insert(optionItem->text(0), optionComboBox->currentText());
            }
        }

        productFilters.insert(productItem->text(0), filter);
    }

    // filter orders
    QList<int> orderIds;

    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);

        // is fulfilled or packaged, skip order
        if (order.isShipped() || order.isPackaged() || order.isRefunded())
            continue;

        // wrong shipping, skip order
        if (!shippingMethods.contains(order.shipping.method) && !anyShipping)
            continue;

        // reset match status for new order
        for (const QString &key : productFilters.keys())
            productFilters[key].matched = false;

        bool isMatch = true;
        for (const Item &item : order.items) {
            // product set as "NOT", skip order
            if (!productFilters.contains(item.product.name)) {
                isMatch = false;
                break;
            }

            ProductFilter &filter = productFilters[item.product.name];

            // qty range not met, skip order
            if ((item.qty < filter.qtyRange.first) || (item.qty > filter.qtyRange.second)) {
                isMatch = false;
                break;
            }

            for (const ItemOption &itemOption : item.options) {
                // Option was removed or renamed, skip option
                if (!filter.options.contains(itemOption.name))
                    continue;

                // option filtered out, skip order
                const bool isAny = (filter.options[itemOption.name] == "*");
                if ((itemOption.choice != filter.options[itemOption.name]) && !isAny) {
                    isMatch = false;
                    break;
                }
            }

            // option mismatch, skip order
            if (!isMatch)
                break;

            filter.matched = true;
        }

        if (!isMatch)
            continue;

        // if even one "AND" filter is not matched, skip order
        for (const ProductFilter &filter : productFilters.values()) {
            if ((filter.logic == MatchLogic::Required) && !filter.matched) {
                isMatch = false;
                break;
            }
        }

        if (!isMatch)
            continue;

        orderIds << id;
    }

    return orderIds;
}
