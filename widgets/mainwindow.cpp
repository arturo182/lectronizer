#include "currencyfetchdialog.h"
#include "enums.h"
#include "mainwindow.h"
#include "markshippeddialog.h"
#include "ordermanager.h"
#include "packaginghelperdialog.h"
#include "settingsdialog.h"
#include "sqlmanager.h"
#include "statisticsdialog.h"
#include "ui_mainwindow.h"
#include "utils.h"

#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QSystemTrayIcon>
#include <QTimer>

MainWindow::MainWindow(SqlManager *sqlMgr, QWidget *parent)
    : QMainWindow{parent}
    , m_firstFetch{true}
    , m_nam{new QNetworkAccessManager(this)}
    , m_orderMgr{new OrderManager(m_nam, &m_shared, sqlMgr, this)}
    , m_sqlMgr{sqlMgr}
    , m_tray(new QSystemTrayIcon(this))
    , m_trayMenu(new QMenu)
    , m_ui{new Ui::MainWindow}
{
    m_ui->setupUi(this);

    m_ui->splitter->setStretchFactor(0, 1);
    m_ui->splitter->setStretchFactor(1, 4);
    m_ui->splitter->setStretchFactor(2, 1);

    m_ui->detailWidget->setOrderManager(m_orderMgr);
    m_ui->detailWidget->setSqlManager(m_sqlMgr);
    m_ui->detailWidget->setSharedData(&m_shared);
    m_ui->detailScroll->hide();

    // Default range to today and don't allow selecting future ranges
    const QDate today = QDate::currentDate();
    m_ui->dateFilterCustomStartEdit->setDate(today);
    m_ui->dateFilterCustomStartEdit->setMaximumDate(today);
    m_ui->dateFilterCustomEndEdit->setDate(today);
    m_ui->dateFilterCustomEndEdit->setMaximumDate(today);

    // Order model stuff
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Id,          new QStandardItem(tr("Id")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::CreatedAt,   new QStandardItem(tr("Created")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Total,       new QStandardItem(tr("Total")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Items,       new QStandardItem(tr("Items")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Customer,    new QStandardItem(tr("Customer")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Country,     new QStandardItem(tr("Country")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Shipping,    new QStandardItem(tr("Shipping")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Status,      new QStandardItem(tr("Status")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::UpdatedAt,   new QStandardItem(tr("Updated")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::FulfilledAt, new QStandardItem(tr("Fulfilled")));
    m_orderModel.setHorizontalHeaderItem((int)ModelColumn::Weight,      new QStandardItem(tr("Weight")));

    m_orderProxyModel.setSourceModel(&m_orderModel);
    m_ui->orderTree->setModel(&m_orderProxyModel);

    // Maybe make these settings later?
    m_ui->filterTree->setOrderModel(&m_orderModel);
    m_ui->filterTree->addFilter(ModelColumn::Country);
    m_ui->filterTree->addFilter(ModelColumn::Shipping);
    m_ui->filterTree->addFilter(ModelColumn::Status);

    // Hide Updated and Fulfilled by default
    m_ui->orderTree->header()->setSectionHidden(8, true);
    m_ui->orderTree->header()->setSectionHidden(9, true);

    // customContextMenuRequested will be called on right click, also for columns
    m_ui->orderTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui->orderTree->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    // Don't repeat, we adjust manually every time
    m_dailySyncTimer.setSingleShot(true);

    setupTrayIcon();
    connectSignals();
    readSettings();

    // sets initial filter
    updateDateFilter();

    QTimer::singleShot(100, this, [&]() { fetchCurrencyRates(); });

    // Force the timer to calculate midnight
    m_dailySyncTimer.start(100);
}

MainWindow::~MainWindow()
{
    delete m_ui;
    m_ui = nullptr;

    delete m_trayMenu;
    m_trayMenu = nullptr;

    delete m_tray;
    m_tray = nullptr;

    delete m_orderMgr;
    m_orderMgr = nullptr;

    delete m_nam;
    m_nam = nullptr;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // hide to tray, if enabled
    if (m_shared.closeToSystemTray && !isHidden() && !m_reallyExit) {
        event->ignore();

        QTimer::singleShot(0, this, &MainWindow::hide);

        if (!m_shared.showedTrayHint) {
            m_tray->showMessage(tr("Lectronizer was closed to system tray"), tr("You can change this in the Settings. This message won't be shown again."));

            m_shared.showedTrayHint = true;
        }

        return;
    }

    writeSettings();
    QMainWindow::closeEvent(event);
    qApp->quit();
}

bool MainWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowStateChange:
    {
        if (isMinimized()) {
            if (m_shared.autoFetchWhenMinimized) {
                m_autoFetchTimer.start();
            }
        } else {
            m_autoFetchTimer.stop();
        }

        break;
    }

    case QEvent::Show:
    {
        m_autoFetchTimer.stop();

        break;
    }

    case QEvent::Hide:
    {
        if (m_shared.autoFetchWhenMinimized)
            m_autoFetchTimer.start();

        break;
    }

    default:
        break;
    }

    return QMainWindow::event(event);
}

void MainWindow::fetchCurrencyRates()
{
    statusBar()->showMessage(tr("Fetching currency rates..."));

    CurrencyFetchDialog dlg(m_nam, this);
    if(dlg.exec() == QDialog::Accepted) {
        statusBar()->showMessage(tr("Currencies fetched"));
    } else {
        statusBar()->showMessage(tr("Currencies disabled"));
    }

    m_shared.currencyRates = dlg.rates();

    QTimer::singleShot(100, m_ui->orderRefreshOrdersAction, &QAction::trigger);
}

void MainWindow::setupTrayIcon()
{
    QAction *showAction = m_trayMenu->addAction(tr("&Hide"));
    m_trayMenu->addAction(QIcon(":/res/icons/arrow_refresh.png"), tr("&Refresh orders"), m_ui->orderRefreshOrdersAction, &QAction::trigger);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(QIcon(":/res/icons/door.png"), tr("&Exit"), this, [this]()
    {
        m_reallyExit = true;
        writeSettings();
        qApp->quit();
    });

    m_trayMenu->setDefaultAction(showAction);

    connect(showAction, &QAction::triggered, this, [this, showAction]()
    {
        if (isHidden()) {
            // make sure we're not minimized
            setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);

            show();
            raise();
            activateWindow();
        } else {
            hide();
        }

        showAction->setText(isVisible() ? tr("&Hide") : tr("&Show"));
    });
    connect(m_tray, &QSystemTrayIcon::messageClicked, showAction, &QAction::trigger);

    connect(m_tray, &QSystemTrayIcon::activated, this, [showAction](const QSystemTrayIcon::ActivationReason &reason)
    {
        if (reason != QSystemTrayIcon::Trigger)
            return;

        showAction->trigger();
    });

    m_tray->setIcon(QIcon(":/res/cart_chip.png"));
    m_tray->setContextMenu(m_trayMenu);
    m_tray->show();
}

void MainWindow::connectSignals()
{
    // Update order details when selection changes
    connect(m_ui->orderTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateOrderRelatedWidgets);

    // Double-clicking an order opens it in a separate window
    connect(m_ui->orderTree, &QTreeView::doubleClicked, this, [this](const QModelIndex &proxyCurrent)
    {
        const int id = orderIdFromProxyModel(proxyCurrent);
        if (id < 0)
            return;

        openOrderWindow(id);
    });

    // Order MenuBar menu
    connect(m_ui->openOrderAction, &QAction::triggered, this, [this]() { openOrderWindow(currentOrderId()); });
    connect(m_ui->markOrderShippedAction, &QAction::triggered, this, [this]()
    {
        const Order &order= m_orderMgr->order(currentOrderId());

        if (order.tracking.required) {
            MarkShippedDialog dlg(order, &m_shared, this);
            if (dlg.exec() == QDialog::Accepted)
                m_orderMgr->markShipped(order.id, dlg.trackingNo(), dlg.trackingUrl());
        } else {
            m_orderMgr->markShipped(order.id);
        }
    });
    connect(m_ui->openOrderInBrowserAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        order.openInBrowser();
    });
    connect(m_ui->openOrderTrackingUrlAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        QDesktopServices::openUrl(QUrl(order.tracking.url));
    });
    connect(m_ui->customerOrderInvoiceAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        QDesktopServices::openUrl(order.customerInvoiceUrl());
    });
    connect(m_ui->sellerOrderInvoiceAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        QDesktopServices::openUrl(order.supplierInvoiceUrl());
    });
    connect(m_ui->copyOrderIdAction, &QAction::triggered, this, [this]() {
        qApp->clipboard()->setText(QString::number(currentOrderId()));
    });
    connect(m_ui->copyOrderCustomerNameAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        const auto &address = order.shipping.address;
        qApp->clipboard()->setText(address.firstName + " " + address.lastName);
    });
    connect(m_ui->copyOrderCustomerEmailAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        qApp->clipboard()->setText(order.customerEmail);
    });
    connect(m_ui->copyOrderFullAddressAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        order.copyFullAddress();
    });
    connect(m_ui->copyOrderTrackingNumberAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        qApp->clipboard()->setText(order.tracking.code);
    });
    connect(m_ui->copyOrderNotesAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        qApp->clipboard()->setText(order.note);
    });
    connect(m_ui->orderFilterCountryAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        m_ui->filterTree->setFilter(tr("Country"),  order.shipping.address.country);
    });
    connect(m_ui->orderFilterShippingAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        m_ui->filterTree->setFilter(tr("Shipping"), order.shipping.method);
    });
    connect(m_ui->orderFilterStatusAction, &QAction::triggered, this, [this]()
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        const Order &order= m_orderMgr->order(id);
        m_ui->filterTree->setFilter(tr("Status"), order.statusString());
    });
    connect(m_ui->orderRefreshOrdersAction, &QAction::triggered, this, [this]()
    {
        statusBar()->showMessage(tr("Refreshing orders..."));
        m_orderMgr->refresh(isHidden() || isMinimized());
    });

    // Order context menu
    connect(m_ui->orderTree, &QHeaderView::customContextMenuRequested, this, [this](const QPoint &pos)
    {
        const int id = currentOrderId();
        if (id < 0)
            return;

        QMenu menu;
        menu.addAction(m_ui->openOrderAction);
        menu.addMenu(m_ui->markOrderPackagedMenu);
        menu.addAction(m_ui->markOrderShippedAction);
        menu.addSeparator();
        menu.addAction(m_ui->openOrderInBrowserAction);
        menu.addAction(m_ui->openOrderTrackingUrlAction);
        menu.addMenu(m_ui->openOrderInvoiceMenu);
        menu.addSeparator();
        menu.addMenu(m_ui->copyOrderMenu);
        menu.addSeparator();
        menu.addMenu(m_ui->orderFilterSameMenu);
        menu.setDefaultAction(m_ui->openOrderAction);
        menu.exec(m_ui->orderTree->mapToGlobal(pos));
    });

    // Update order details and number info when filters change
    connect(&m_orderProxyModel, &OrderSortFilterModel::layoutChanged, this, &MainWindow::updateOrderRelatedWidgets);

    // Dialogs
    connect(m_ui->toolsPackagingHelperAction, &QAction::triggered, this, [this]()
    {
        PackagingHelperDialog dlg(m_orderMgr, m_sqlMgr, this);
        dlg.exec();
    });
    connect(m_ui->toolsStatisticsAction, &QAction::triggered, this, [this]()
    {
        if (m_orderMgr->orderIds().size() == 0) {
            QMessageBox::information(this, tr("Not enough data"), tr("Sorry, can't show stats until there is at least one order."));
            return;
        }

        StatisticsDialog dlg(m_orderMgr, m_sqlMgr, this);
        dlg.exec();
    });
    connect(m_ui->toolsSettingsAction, &QAction::triggered, this, &MainWindow::showSettingsDialog);
    connect(m_ui->helpAboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    // Pass filter changes to the filter model
    connect(m_ui->filterTree, &FilterTreeWidget::filterChanged, &m_orderProxyModel, &OrderSortFilterModel::setColumnFilters);

    // Date range filter widgets
    connect(m_ui->dateFilterGroup,           &QGroupBox::toggled, this, &MainWindow::updateDateFilter);
    connect(m_ui->dateFilterTodayRadio,      &QRadioButton::toggled, this, &MainWindow::updateDateFilter);
    connect(m_ui->dateFilterYesterdayRadio,  &QRadioButton::toggled, this, &MainWindow::updateDateFilter);
    connect(m_ui->dateFilter7DaysRadio,      &QRadioButton::toggled, this, &MainWindow::updateDateFilter);
    connect(m_ui->dateFilter30DaysRadio,     &QRadioButton::toggled, this, &MainWindow::updateDateFilter);
    connect(m_ui->dateFilterCustomRadio,     &QRadioButton::toggled, this, &MainWindow::updateDateFilter);
    connect(m_ui->dateFilterCustomStartEdit, &QDateEdit::dateChanged, this, &MainWindow::updateDateFilter);
    connect(m_ui->dateFilterCustomEndEdit,   &QDateEdit::dateChanged, this, &MainWindow::updateDateFilter);

    // Search bar
    connect(m_ui->orderSearchEdit, &QLineEdit::textChanged, this, [this](const QString &text)
    {
        m_orderProxyModel.setFilterFixedString(text);
        m_orderProxyModel.invalidate();
    });

    const auto createColumnMenu = [this](QMenu *menu)
    {
        QHeaderView *header = m_ui->orderTree->header();

        const int columnCount = m_orderModel.columnCount();
        for (int i = 0; i < columnCount; ++i) {
            const QString name = m_orderModel.headerData(i, Qt::Horizontal).toString();

            QAction *action = menu->addAction(name, this, [header, i]()
            {
                // If we hide all the header goes away, let's not
                if ((header->count() - header->hiddenSectionCount() == 1) && !header->isSectionHidden(i))
                    return;

                header->setSectionHidden(i, !header->isSectionHidden(i));
            });
            action->setCheckable(true);
            action->setChecked(!header->isSectionHidden(i));
        }
    };

    // Column context menu
    connect(m_ui->orderTree->header(), &QHeaderView::customContextMenuRequested, this, [this, createColumnMenu](const QPoint &pos)
    {
        QHeaderView *header = m_ui->orderTree->header();

        QMenu menu;
        createColumnMenu(&menu);

        menu.exec(header->mapToGlobal(pos));
    });

    // Update order detail buttons when sorting changes
    connect(m_ui->orderTree->header(), &QHeaderView::sortIndicatorChanged, this, [this]()
    {
        QItemSelectionModel *selection = m_ui->orderTree->selectionModel();
        if (!selection->hasSelection())
            return;

        const QModelIndex proxyCurrent = selection->currentIndex();
        m_ui->detailWidget->updateOrderButtons(proxyCurrent.row(), m_orderProxyModel.rowCount());
    });

    // Column menubar menu
    connect(m_ui->viewColumnsMenu, &QMenu::aboutToShow, this, [this, createColumnMenu]()
    {
        m_ui->viewColumnsMenu->clear();
        createColumnMenu(m_ui->viewColumnsMenu);
    });

    // Process Prev/Next buttons in Order details panel
    connect(m_ui->detailWidget, &OrderDetailsWidget::orderRequested, this, [this](const int delta)
    {
        QTreeView *tree = m_ui->orderTree;
        QItemSelectionModel *selection = tree->selectionModel();
        if (!selection->hasSelection())
            return;

        const bool isNext = (delta == 1);
        const int currentRow = selection->currentIndex().row();
        const int totalRows = tree->model()->rowCount();

        QModelIndex newIndex;
        if (isNext && (currentRow < totalRows - 1)) {
            newIndex = tree->model()->index(currentRow + 1, 0);
        } else if (!isNext && (currentRow > 0)) {
            newIndex = tree->model()->index(currentRow - 1, 0);
        }

        if (newIndex.isValid())
            selection->setCurrentIndex(newIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    });

    // Colapse details widget
    connect(m_ui->detailWidget, &OrderDetailsWidget::hideRequested, m_ui->detailScroll, &QWidget::hide);

    // New and updated orders
    connect(m_orderMgr, &OrderManager::orderReceived, this, &MainWindow::addOrder);
    connect(m_orderMgr, &OrderManager::orderUpdated, this, &MainWindow::updateOrder);
    connect(m_orderMgr, &OrderManager::refreshCompleted, this, [this](const int newOrders, const int updatedOrders)
    {
       m_ui->filterTree->refreshFilters();

       // refresh the details in case anything got updated
       updateOrderRelatedWidgets();

       statusBar()->showMessage(tr("Orders refreshed, %1 new, %2 updated").arg(newOrders).arg(updatedOrders), 5000);

       if (!m_firstFetch) {
           if (newOrders > 0) {
               m_tray->showMessage((newOrders == 1) ? tr("New order!") : tr("New orders!"),
                                   tr("%n new order(s) received", "", newOrders),
                                   QSystemTrayIcon::Information,
                                   5000);
           } else if (updatedOrders > 0) {
               m_tray->showMessage((updatedOrders == 1) ? tr("Order updated!") : tr("Orders updated!"),
                                   tr("%n order(s) have been updated", "", updatedOrders),
                                   QSystemTrayIcon::Information,
                                   5000);
           }
       }

       m_firstFetch = false;
    });
    connect(m_orderMgr, &OrderManager::refreshFailed, this, [this](const QString &errorStr)
    {
        statusBar()->showMessage(tr("Order refresh failed: %1").arg(errorStr), 5000);
    });

    // Auto refresh timer
    connect(&m_autoFetchTimer, &QTimer::timeout, m_ui->orderRefreshOrdersAction, &QAction::trigger);

    // Daily row sync timer
    connect(&m_dailySyncTimer, &QTimer::timeout, [this]()
    {
        syncAllOrderRows();

        const QDateTime midnight(QDate::currentDate().addDays(1), QTime(0, 0));
        const QDateTime now = QDateTime::currentDateTime();

        const qint64 msecs = midnight.toMSecsSinceEpoch() - now.toMSecsSinceEpoch();

        m_dailySyncTimer.start(msecs);
    });
}

void MainWindow::readSettings()
{
    QSettings set;

    restoreGeometry(set.value("geometry").toByteArray());
    restoreState(set.value("state").toByteArray());

    m_ui->orderTree->header()->restoreState(set.value("orderColumns").toByteArray());
    m_ui->splitter->restoreState(set.value("splitter").toByteArray());

    m_shared.apiKey                 = set.value("apiKey").toString();
    m_shared.targetCurrency         = set.value("targetCurrency", "EUR").toString();
    m_shared.closeToSystemTray      = set.value("closeToSystemTray", true).toBool();
    m_shared.showedTrayHint         = set.value("showedTrayHint").toBool();
    m_shared.autoFetchWhenMinimized = set.value("autoFetchWhenMinimized").toBool();
    m_shared.autoFetchIntervalMin   = set.value("autoFetchIntervalMin").toInt();
    m_shared.trackingUrl            = set.value("trackingUrl").toString();

    m_ui->detailWidget->readSettings(set);
    m_ui->filterTree->readSettings();

    updateAutoFetchTimer();
}

void MainWindow::writeSettings() const
{
    QSettings set;

    set.setValue("geometry", saveGeometry());
    set.setValue("state",    saveState());
    set.setValue("orderColumns",  m_ui->orderTree->header()->saveState());
    set.setValue("splitter",  m_ui->splitter->saveState());

    set.setValue("apiKey",                  m_shared.apiKey);
    set.setValue("targetCurrency",          m_shared.targetCurrency);
    set.setValue("closeToSystemTray",       m_shared.closeToSystemTray);
    set.setValue("showedTrayHint",          m_shared.showedTrayHint);
    set.setValue("autoFetchWhenMinimized",  m_shared.autoFetchWhenMinimized);
    set.setValue("autoFetchIntervalMin",    m_shared.autoFetchIntervalMin);
    set.setValue("trackingUrl",             m_shared.trackingUrl);

    m_ui->detailWidget->writeSettings(set);
    m_ui->filterTree->writeSettings();
}

void MainWindow::openOrderWindow(const int id)
{
    if (id < 0)
        return;

    OrderDetailsWidget *orderWidget = new OrderDetailsWidget(true);
    orderWidget->setAttribute(Qt::WA_DeleteOnClose);
    orderWidget->setOrderManager(m_orderMgr);
    orderWidget->setSqlManager(m_sqlMgr);
    orderWidget->setSharedData(&m_shared);
    orderWidget->setOrder(m_orderMgr->order(id));
    orderWidget->show();
}

int MainWindow::orderIdFromProxyModel(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return -1;

    const QModelIndex index = m_orderProxyModel.mapToSource(proxyIndex);
    if (!index.isValid())
        return -1;

    QStandardItem *item = m_orderModel.item(index.row());
    if (!item)
        return -1;

    return item->text().toInt();
}

int MainWindow::currentOrderId() const
{
    QTreeView *tree = m_ui->orderTree;
    QItemSelectionModel *selection = tree->selectionModel();
    if (!selection->hasSelection())
        return -1;

    return orderIdFromProxyModel(selection->currentIndex());
}

void MainWindow::syncOrderRow(const int row, const Order &order)
{
    const auto setColumn = [this, row](const ModelColumn &column, const QString &text, const QVariant &data = {})
    {
        QStandardItem *item = m_orderModel.item(row, (int)column);
        item->setText(text);
        item->setData(data);
    };

    setColumn(ModelColumn::Id, QString::number(order.id));

    setColumn(ModelColumn::CreatedAt, friendlyDate(order.createdAt), order.createdAt);

    const QString converted = convertCurrencyString(order.total);
    setColumn(ModelColumn::Total, QString("%1 %2%3")
                                  .arg(order.total)
                                  .arg(order.currency)
                                  .arg(converted.isEmpty() ? "" : " (" + converted + ")"), order.total);

    setColumn(ModelColumn::Items, order.itemListing());

    setColumn(ModelColumn::Customer, QString("%1 %2").arg(order.shipping.address.firstName, order.shipping.address.lastName));

    setColumn(ModelColumn::Country, order.shipping.address.country);

    setColumn(ModelColumn::Shipping, order.shipping.method);

    setColumn(ModelColumn::Status, order.statusString());

    setColumn(ModelColumn::UpdatedAt, friendlyDate(order.updatedAt), order.updatedAt);

    if (order.fulfilledAt.isValid()) {
        setColumn(ModelColumn::FulfilledAt, friendlyDate(order.fulfilledAt), order.fulfilledAt);
    } else {
        setColumn(ModelColumn::FulfilledAt, "-");
    }

    setColumn(ModelColumn::Weight, tr("%1 %2").arg(order.calcWeight(), 0, 'f', 1).arg(order.weight.unit), order.calcWeight());
}

void MainWindow::syncAllOrderRows()
{
    for (int row = 0; row < m_orderModel.rowCount(); ++row) {
        QStandardItem *idItem = m_orderModel.item(row, 0);
        const int id = idItem->text().toInt();

        syncOrderRow(row, m_orderMgr->order(id));
    }
}

double MainWindow::convertCurrency(const double eur)
{
    if (m_shared.targetCurrency == "EUR")
        return -1.0;

    if (!m_shared.currencyRates.contains(m_shared.targetCurrency))
        return -1.0;

    return eur * m_shared.currencyRates[m_shared.targetCurrency];
}

QString MainWindow::convertCurrencyString(const double eur)
{
    const double converted = convertCurrency(eur);
    if (converted < 0)
        return QString();

    return QString("%1 %2")
                .arg(converted, 0, 'f', 2)
                .arg(m_shared.targetCurrency);
}

void MainWindow::addOrder(const Order &order)
{
    // insert a row of empty items and then sync the order
    QList<QStandardItem*> rowItems;
    for (int i = 0; i < (int)ModelColumn::LastValue; ++i)
        rowItems << new QStandardItem();

    QStandardItem *rootItem = m_orderModel.invisibleRootItem();
    rootItem->appendRow(rowItems);

    syncOrderRow(rootItem->rowCount() - 1, order);

    updateTreeStatsLabel();
}

void MainWindow::updateOrder(const Order &order)
{
    for (int row = 0; row < m_orderModel.rowCount(); ++row) {
        QStandardItem *idItem = m_orderModel.item(row, 0);
        const int id = idItem->text().toInt();

        if (id != order.id)
            continue;

        syncOrderRow(row, order);

        break;
    }

    m_ui->filterTree->refreshFilters();

    if (!m_ui->detailScroll->isVisible())
        return;

    QItemSelectionModel *selection = m_ui->orderTree->selectionModel();
    if (selection->hasSelection()) {
        const QModelIndex proxyCurrent = selection->selection().indexes().first();
        const int id = orderIdFromProxyModel(proxyCurrent);
        if (id == order.id)
            updateOrderDetails(selection->selection());
    }
}

void MainWindow::updateDateFilter()
{
    QDateTime startDate;
    QDateTime endDate;

    const auto setRange = [&](const QDate &start, const QDate &end)
    {
        startDate = QDateTime(start, QTime(0, 0, 0));
        endDate   = QDateTime(end, QTime(23, 59, 59));
    };

    const QDate today = QDate::currentDate();

    const bool groupEnabled = m_ui->dateFilterGroup->isChecked();
    const bool isCustomRange = m_ui->dateFilterCustomRadio->isChecked();

    m_ui->dateFilterCustomStartEdit->setEnabled(groupEnabled && isCustomRange);
    m_ui->dateFilterCustomEndEdit->setEnabled(groupEnabled && isCustomRange);

    if (!groupEnabled) {
        setRange(QDate(2000, 1, 1), QDate(2345, 6, 7)); // Fix in 300 years
    } else if (m_ui->dateFilterTodayRadio->isChecked()) {
        setRange(today, today);
    } else if (m_ui->dateFilterYesterdayRadio->isChecked()) {
        const QDate yesterday = today.addDays(-1);
        setRange(yesterday, yesterday);
    } else if (m_ui->dateFilter7DaysRadio->isChecked()) {
        const QDate sevenAgo = today.addDays(-6);
        setRange(sevenAgo, today);
    } else if (m_ui->dateFilter30DaysRadio->isChecked()) {
        const QDate thirtyAgo = today.addDays(-29);
        setRange(thirtyAgo, today);
    } else if (isCustomRange) {
        setRange(m_ui->dateFilterCustomStartEdit->date(), m_ui->dateFilterCustomEndEdit->date());
    }

    m_orderProxyModel.setDateFilter(startDate, endDate);
}

void MainWindow::updateOrderDetails(const QItemSelection &selected)
{
    if (selected.indexes().isEmpty()) {
        m_ui->detailScroll->hide();
        return;
    }

    const QModelIndex proxyCurrent = selected.indexes().first();
    const int id = orderIdFromProxyModel(proxyCurrent);
    if (id < 0)
        return;

    const Order &order = m_orderMgr->order(id);
    m_ui->detailWidget->setOrder(order);

    const int row = proxyCurrent.row();
    const int rowCount = m_orderProxyModel.rowCount();
    m_ui->detailWidget->updateOrderButtons(row, rowCount);

    m_ui->detailScroll->show();
}

void MainWindow::updateOrderRelatedWidgets()
{
    bool hasSelection = false;
    bool trackingRequired = false;
    bool hasTrackingUrl = false;
    bool hasTrackingCode = false;
    bool hasNote = false;
    bool isFulfilled = false;
    bool isRefunded = false;
    int packagingId = -1;

    QItemSelectionModel *selection = m_ui->orderTree->selectionModel();
    if (!selection->hasSelection()) { // no order, just hide details
        m_ui->detailScroll->hide();
    } else {
        updateOrderDetails(selection->selection());

        // get selected order and determine needed properties
        const QModelIndex proxyCurrent = selection->selection().indexes().first();
        const int orderId = orderIdFromProxyModel(proxyCurrent);
        if (orderId >= 0) {
            const Order &order = m_orderMgr->order(orderId);

            trackingRequired = order.tracking.required;
            hasTrackingCode  = !order.tracking.code.isEmpty();
            hasTrackingUrl   = !order.tracking.url.isEmpty();
            hasNote          = !order.note.isEmpty();
            isFulfilled      = order.isShipped();
            isRefunded       = order.isRefunded();
            packagingId      = order.packaging;
            hasSelection     = true;
        }

        // generate packaging menu actions
        m_ui->markOrderPackagedMenu->clear();
        if (packagingId < 0) {
            const auto packageOrder = [this, orderId](const int packId)
            {
                if (packId > 0) {
                    const int stock = m_sqlMgr->packagingStock(packId);
                    m_sqlMgr->setPackagingStock(packId, (stock > 0) ? (stock - 1) : stock);
                }

                Order &order = m_orderMgr->order(orderId);
                order.packaging = packId;
                emit m_orderMgr->orderUpdated(order);
            };

            // default packaging
            m_ui->markOrderPackagedMenu->addAction(tr("Default packaging"), this, [packageOrder]() { packageOrder(0); });

            // user packagings
            const QList<Packaging> packagingTypes = m_sqlMgr->packagings();
            for (int i = 0; i < packagingTypes.size(); ++i) {
                const Packaging &pack = packagingTypes[i];
                const int packId = pack.id;

                m_ui->markOrderPackagedMenu->addAction(tr("%1 (%2 left)").arg(pack.name).arg(pack.stock), this, [packageOrder, packId]() { packageOrder(packId); });
            }
        }
    }

    // update menu actions as needed
    m_ui->openOrderAction->setEnabled(hasSelection);
    m_ui->markOrderShippedAction->setEnabled(hasSelection && !isFulfilled && !isRefunded);
    m_ui->markOrderPackagedMenu->setEnabled(hasSelection && (packagingId < 0));
    m_ui->openOrderInBrowserAction->setEnabled(hasSelection);
    m_ui->openOrderTrackingUrlAction->setEnabled(hasSelection && trackingRequired && hasTrackingUrl);
    m_ui->openOrderInvoiceMenu->setEnabled(hasSelection);
    m_ui->copyOrderMenu->setEnabled(hasSelection);
    m_ui->copyOrderTrackingNumberAction->setEnabled(hasSelection && trackingRequired && hasTrackingCode);
    m_ui->copyOrderNotesAction->setEnabled(hasSelection && hasNote);
    m_ui->orderFilterSameMenu->setEnabled(hasSelection);

    updateTreeStatsLabel();
}

void MainWindow::updateTreeStatsLabel()
{
    m_ui->treeStatsLabel->setText(tr("Showing %1 of %2")
                                    .arg(m_orderProxyModel.rowCount())
                                    .arg(m_orderModel.rowCount()));
}

void MainWindow::updateAutoFetchTimer()
{
    m_autoFetchTimer.setInterval(m_shared.autoFetchIntervalMin * 60 * 1000);
    m_autoFetchTimer.setSingleShot(false);
}

void MainWindow::showSettingsDialog()
{
    SettingsDialog dlg(m_orderMgr, m_sqlMgr, this);
    connect(&dlg, &SettingsDialog::applied, this, [this, &dlg]()
    {
        m_shared = dlg.data();

        syncAllOrderRows();
        updateOrderRelatedWidgets();
        updateAutoFetchTimer();
    });

    dlg.setData(m_shared);
    dlg.exec();
}

void MainWindow::showAboutDialog()
{
    const auto compilerVersion = []() -> QString
    {
#if defined(Q_CC_CLANG)
        return "Clang " + QString::number(__clang_major__) + "." + QString::number(__clang_minor__) +
#if defined(__apple_build_version__)
                        " (Apple)"
#else
                        ""
#endif
                        ;
#elif defined(Q_CC_GNU)
        return QString("GCC %1").arg(__VERSION__);
#elif defined(Q_CC_MSVC)
if (_MSC_VER >= 1800) // 1800: MSVC 2013 (yearly release cycle)
        return "MSVC " + QString::number(2008 + ((_MSC_VER / 100) - 13));
if (_MSC_VER >= 1500) // 1500: MSVC 2008, 1600: MSVC 2010, ... (2-year release cycle)
        return "MSVC " + QString::number(2008 + 2 * ((_MSC_VER / 100) - 15));
#else
        return "<unknown compiler>";
#endif
    };

    QString verSha1;
#ifdef VER_SHA1
    verSha1 = QString("From revision <a href='https://github.com/arturo182/lectronizer/commit/%1'>%1</a><br><br>").arg(VER_SHA1);
#endif

    const QString description = tr("<h3>%1 v%2</h3>"
                                   "Based on Qt %3 (%4)<br><br>"
                                   "Built on %5 %6<br><br>"
                                   "%7"
                                   "Copyright &copy; 2022 <a href='http://twitter.com/arturo182'>arturo182</a>. All rights reserved.<br><br>"
                                   "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, "
                                   "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.");

    QMessageBox::about(this, tr("About %1").arg(qApp->applicationName()),
                       description
                           .arg(qApp->applicationName())
                           .arg(qApp->applicationVersion())
                           .arg(qVersion())
                           .arg(compilerVersion())
                           .arg(__DATE__)
                           .arg(__TIME__)
                       .arg(verSha1));
}
