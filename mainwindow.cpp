#include "mainwindow.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"
#include "utils.h"

#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QItemDelegate>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QSettings>
#include <QSslSocket>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QUrlQuery>

static const QString ApiUrl{"https://lectronz.com/api/v1/orders"};
static const QString CurrencyUrl{"https://api.exchangerate.host/latest"};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    m_ui->splitter->setStretchFactor(0, 1);
    m_ui->splitter->setStretchFactor(1, 4);
    m_ui->splitter->setStretchFactor(2, 1);

    m_ui->detailWidget->setSharedData(&m_shared);
    m_ui->detailScroll->hide();

    // Default range to today and don't allow selecting future ranges
    const QDate today = QDate::currentDate();
    m_ui->dateFilterCustomStartEdit->setDate(today);
    m_ui->dateFilterCustomStartEdit->setMaximumDate(today);
    m_ui->dateFilterCustomEndEdit->setDate(today);
    m_ui->dateFilterCustomEndEdit->setMaximumDate(today);

    // Order model stuff
    m_orderModel.setHorizontalHeaderLabels({
                                               tr("Id"),        // 0
                                               tr("Created"),   // 1
                                               tr("Total"),     // 2
                                               tr("Items"),     // 3
                                               tr("Customer"),  // 4
                                               tr("Country"),   // 5
                                               tr("Shipping"),  // 6
                                               tr("Status"),    // 7
                                               tr("Updated"),   // 8
                                               tr("Fulfilled"), // 9
                                               tr("Weight"),    // 10
                                           });

    m_orderProxyModel.setSourceModel(&m_orderModel);
    m_ui->orderTreeView->setModel(&m_orderProxyModel);

    // Hide Updated and Fulfilled by default
    m_ui->orderTreeView->header()->setSectionHidden(8, true);
    m_ui->orderTreeView->header()->setSectionHidden(9, true);

    // customContextMenuRequested will be called on right click, also for columns
    m_ui->orderTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui->orderTreeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    m_ui->orderTreeView->installEventFilter(this);

    connectSignals();
    readSettings();

    // sets initial filter
    updateDateFilter();

    QTimer::singleShot(100, this, [&]() { fetchCurrencyRates(); });
}

MainWindow::~MainWindow()
{
    delete m_ui;
    m_ui = nullptr;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();

    QMainWindow::closeEvent(event);

    qApp->quit();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if ((object == m_ui->orderTreeView) && (event->type() == QEvent::KeyRelease))
        orderTreeKeyPressEvent(static_cast<QKeyEvent*>(event));

    return QMainWindow::eventFilter(object, event);
}

void MainWindow::addColumnFilter(const int column, const QString &name, std::function<QString(const Order &)> getter)
{
    QStringList items;

    QHashIterator<int, Order> it(m_orders);
    while (it.hasNext()) {
        it.next();

        const Order &order = it.value();
        items << getter(order);
    }

    items.removeDuplicates();
    items.sort(Qt::CaseInsensitive);

    m_ui->filterTreeWidget->addFilter(column, name, items);
}

// TODO: Move to separate Fetcher class with signals and such
void MainWindow::fetchOrders(const int offset, const int limit)
{
    if (m_shared.apiKey.isEmpty()) {
        statusBar()->showMessage(tr("No API key set up, go to Tools->Settings..."));
        QMessageBox::warning(this, tr("Setup needed"), tr("You need to fill in your API key first. Go to Tools->Settings..., paste it there and then restart the app."));
        return;
    }

    statusBar()->showMessage(tr("Fetching orders..."));

    QUrlQuery query;
    query.addQueryItem("offset", QString::number(offset));
    query.addQueryItem("limit", QString::number(limit));

    QUrl url(ApiUrl);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("Accept", "*/*");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(m_shared.apiKey).toUtf8());

    QNetworkReply *reply = m_nam->get(req);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(reply, &QNetworkReply::errorOccurred, reply, [this, reply]()
#else
    connect(reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), reply, [this, reply]()
#endif
    {
        QMessageBox::critical(this, tr("Error fetching orders"), reply->errorString());
        reply->deleteLater();
    });
    connect(reply, &QNetworkReply::sslErrors, reply, [this, reply](const QList<QSslError> &errors)
    {
        QMessageBox::critical(this, tr("Error fetching orders - SSL"), errors[0].errorString());
        reply->deleteLater();
    });
    connect(reply, &QNetworkReply::finished, reply, [&, reply]()
    {
        const QByteArray json = reply->readAll();

        QJsonParseError error = {};
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        if (error.error != QJsonParseError::NoError) {
            QMessageBox::critical(this, tr("Error parsing orders"), error.errorString());
            reply->deleteLater();
            return;
        }

        const QJsonObject root = doc.object();
        const QJsonArray jsonOrders = root.value("orders").toArray();

        m_orderOffset = root.value("offset").toInt();
        m_totalOrders = root.value("total_count").toInt();

        m_orders.clear();
        for (const QJsonValue &val : jsonOrders) {
            const Order order = parseJsonOrder(val);
            m_orders.insert(order.id, order);
        }

        QStandardItem *rootItem = m_orderModel.invisibleRootItem();
        rootItem->removeRows(0, m_orderModel.rowCount());

        QHashIterator<int, Order> it(m_orders);
        while (it.hasNext()) {
            it.next();

            const Order &order = it.value();

            QList<QStandardItem*> rowItems;
            rowItems << new QStandardItem(QString::number(order.id));

            rowItems << new QStandardItem(friendlyDate(order.createdAt));
            rowItems.last()->setData(order.createdAt);

            QString totalTargetCurrency;
            if (m_shared.targetCurrency != "EUR")
                totalTargetCurrency = QString(" (%1 %2)")
                                        .arg(order.total * m_shared.currencyRates[m_shared.targetCurrency], 0, 'f', 2)
                                        .arg(m_shared.targetCurrency);

            rowItems << new QStandardItem(QString("%1 %2%3")
                                          .arg(order.total)
                                          .arg(order.currency)
                                          .arg(totalTargetCurrency));
            rowItems.last()->setData(order.total);

            QString itemText;
            for (const Item &item : order.items) {
                itemText += QString("- %1x %2\n").arg(item.qty).arg(item.product.name);

                if (item.options.isEmpty())
                    continue;

                for (int i = 0; i < item.options.count(); ++i) {
                    itemText += QString("    - %1 = %2").arg(item.options[i].name, item.options[i].choice);

                    if (i < item.options.count() - 1)
                        itemText += "\n";
                }
            }

            rowItems << new QStandardItem(itemText);
            rowItems << new QStandardItem(QString("%1 %2").arg(order.shipping.address.firstName, order.shipping.address.lastName));
            rowItems << new QStandardItem(order.shipping.address.country);
            rowItems << new QStandardItem(order.shipping.method);
            rowItems << new QStandardItem(order.status);

            rowItems << new QStandardItem(friendlyDate(order.updatedAt));
            rowItems.last()->setData(order.updatedAt);

            if (order.fulfilledAt.isValid()) {
                rowItems << new QStandardItem(friendlyDate(order.fulfilledAt));
                rowItems.last()->setData(order.fulfilledAt);
            } else {
                rowItems << new QStandardItem("-");
            }

            rowItems << new QStandardItem(tr("%1 %2").arg(order.calcWeight(), 0, 'f', 1).arg(order.weight.unit));
            rowItems.last()->setData(order.calcWeight());

            rootItem->appendRow(rowItems);
        }
        m_ui->orderTreeView->setSortingEnabled(true);

        addColumnFilter(5, tr("Country"),  [](const Order &order) { return order.shipping.address.country; });
        addColumnFilter(6, tr("Shipping"), [](const Order &order) { return order.shipping.method; });
        addColumnFilter(7, tr("Status"),   [](const Order &order) { return order.status; });

        m_ui->orderNumberLabel->setText(tr("%1-%2 of %3")
                                        .arg(1)
                                        .arg(m_orders.size())
                                        .arg(m_totalOrders));

        statusBar()->showMessage(tr("Orders fetched"), 3000);

        reply->deleteLater();
    });
}

void MainWindow::fetchCurrencyRates()
{
    statusBar()->showMessage(tr("Fetching currency rates..."));

    QNetworkRequest req(CurrencyUrl);
    QNetworkReply *reply = m_nam->get(req);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(reply, &QNetworkReply::errorOccurred, reply, [this, reply]()
#else
    connect(reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), reply, [this, reply]()
#endif
    {
        QMessageBox::critical(this, tr("Error fetching currencies"), reply->errorString());
        reply->deleteLater();
    });
    connect(reply, &QNetworkReply::sslErrors, reply, [&, reply](const QList<QSslError> &errors)
    {
        QMessageBox::critical(this, tr("Error fetching currencies - SSL"), errors[0].errorString());
        reply->deleteLater();
    });
    connect(reply, &QNetworkReply::finished, reply, [&, reply]()
    {
        const QByteArray json = reply->readAll();

        QJsonParseError error = {};
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        if (error.error != QJsonParseError::NoError) {
            QMessageBox::critical(this, tr("Error parsing currencies"), error.errorString());

            reply->deleteLater();
            return;
        }

        const QJsonObject root = doc.object();

        if (root.value("base").toString() != "EUR") {
            QMessageBox::critical(this, tr("Unexpected value"), "Base currency is not EUR");
            reply->deleteLater();
            return;
        }

        const QJsonObject rates = root.value("rates").toObject();
        for (auto it = rates.begin(); it != rates.end(); ++it)
            m_shared.currencyRates.insert(it.key(), it.value().toDouble());

        QTimer::singleShot(100, this, [&]() { fetchOrders(); });

        statusBar()->showMessage(tr("Currencies fetched"));

        reply->deleteLater();
    });
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

    const Order &order = m_orders[id];
    m_ui->detailWidget->setOrder(order);

    const int row = proxyCurrent.row();
    const int rowCount = m_orderProxyModel.rowCount();
    m_ui->detailWidget->updateOrderButtons(row, rowCount);

    m_ui->detailScroll->show();
}

void MainWindow::readSettings()
{
    QSettings set;

    restoreGeometry(set.value("geometry").toByteArray());
    restoreState(set.value("state").toByteArray());

    m_ui->orderTreeView->header()->restoreState(set.value("orderColumns").toByteArray());
    m_ui->splitter->restoreState(set.value("splitter").toByteArray());

    m_shared.apiKey = set.value("apiKey").toString();
    m_shared.targetCurrency = set.value("targetCurrency", "EUR").toString();

    m_ui->detailWidget->readSettings(set);
}

void MainWindow::writeSettings() const
{
    QSettings set;

    set.setValue("geometry", saveGeometry());
    set.setValue("state",    saveState());
    set.setValue("orderColumns",  m_ui->orderTreeView->header()->saveState());
    set.setValue("splitter",  m_ui->splitter->saveState());
    set.setValue("apiKey", m_shared.apiKey);
    set.setValue("targetCurrency", m_shared.targetCurrency);

    m_ui->detailWidget->writeSettings(set);
}

void MainWindow::connectSignals()
{
    // Update order details when selection changes
    connect(m_ui->orderTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::updateOrderDetails);

    // Double-clicking an order opens it in a separate window
    connect(m_ui->orderTreeView, &QTreeView::doubleClicked, this, [&](const QModelIndex &proxyCurrent)
    {
        const int id = orderIdFromProxyModel(proxyCurrent);
        if (id < 0)
            return;

        openOrderWindow(id);
    });

    // Order context menu
    connect(m_ui->orderTreeView, &QHeaderView::customContextMenuRequested, this, [&](const QPoint &pos)
    {
        QTreeView *tree = m_ui->orderTreeView;
        QItemSelectionModel *selection = tree->selectionModel();
        if (!selection->hasSelection())
            return;

        const int id = orderIdFromProxyModel(selection->currentIndex());
        if (id < 0)
            return;

        const Order &order= m_orders[id];

        QMenu menu;
        QAction *openAction = menu.addAction(tr("&Open"), this, [&, id]() { openOrderWindow(id); }, Qt::Key_Return);
        menu.setDefaultAction(openAction);

        QAction *markShippedAction = menu.addAction(QIcon(":/res/icons/package_go.png"), tr("Mark &shipped"), this, [&, id]()
        {
            QMessageBox::information(this, tr("Not implemented"), tr("Not implemented yet, sorry"));
        }, QKeySequence("Ctrl+Return"));
        markShippedAction->setEnabled(!order.fulfilledAt.isValid());

        menu.addSeparator();

        menu.addAction(QIcon(":/res/icons/world.png"), tr("Open in &browser"), this, [&, order]() { order.openInBrowser(); });
        QAction *openTrackingAction = menu.addAction(QIcon(":/res/icons/package_link.png"), tr("Open &tracking URL"), this, [&, order]()
        {
            QDesktopServices::openUrl(QUrl(order.tracking.url));
        });
        openTrackingAction->setEnabled(order.tracking.required && !order.tracking.url.isEmpty());

        QMenu *invoiceMenu = menu.addMenu(QIcon(":/res/icons/page_money.png"), tr("Open &invoice"));
        invoiceMenu->addAction(QIcon(), tr("&Customer invoice"), this, [&, order]() { QDesktopServices::openUrl(order.customerInvoiceUrl()); });
        invoiceMenu->addAction(QIcon(), tr("&Seller invoice"),   this, [&, order]() { QDesktopServices::openUrl(order.supplierInvoiceUrl()); });

        menu.addSeparator();

        QMenu *copyMenu = menu.addMenu(QIcon(":/res/icons/page_white_copy.png"), tr("&Copy"));
        copyMenu->addAction(QIcon(":/res/icons/box.png"),   tr("Order &Id"),              this, [&, order]() { qApp->clipboard()->setText(QString::number(order.id)); });
        copyMenu->addAction(QIcon(":/res/icons/user.png"),  tr("Customer &Name"),         this, [&, order]() { const auto &a = order.shipping.address; qApp->clipboard()->setText(a.firstName + " " + a.lastName); });
        copyMenu->addAction(QIcon(":/res/icons/email.png"), tr("Customer &Email"),        this, [&, order]() { qApp->clipboard()->setText(order.customerEmail); });
        copyMenu->addAction(QIcon(":/res/icons/house.png"), tr("Full shipping &address"), this, [&, order]() { order.copyFullAddress(); });
        QAction *copyTrackingAction = copyMenu->addAction(QIcon(":/res/icons/package.png"), tr("Copy &tracking number"), this, [&, order]() { qApp->clipboard()->setText(order.tracking.code); });
        copyTrackingAction->setEnabled(order.tracking.required && !order.tracking.code.isEmpty());

        menu.addSeparator();

        QMenu *filterMenu = menu.addMenu(QIcon(":/res/icons/funnel.png"), tr("&Filter with same"));
        filterMenu->addAction(tr("&Country"),  this, [&, order]() { m_ui->filterTreeWidget->setFilter(tr("Country"), order.shipping.address.country); });
        filterMenu->addAction(tr("&Shipping"), this, [&, order]() { m_ui->filterTreeWidget->setFilter(tr("Shipping"), order.shipping.method); });
        filterMenu->addAction(tr("S&tatus"),   this, [&, order]() { m_ui->filterTreeWidget->setFilter(tr("Status"), order.status); });

        menu.exec(tree->mapToGlobal(pos));
    });

    // Update order details when filters change
    connect(&m_orderProxyModel, &OrderSortFilterModel::layoutChanged, this, [&]()
    {
        QItemSelectionModel *selection = m_ui->orderTreeView->selectionModel();
        if (!selection->hasSelection()) {
            m_ui->detailScroll->hide();
            return;
        }
        updateOrderDetails(selection->selection());
    });

    // Dialogs
    connect(m_ui->toolsSettingsAction, &QAction::triggered, this, &MainWindow::showSettingsDialog);
    connect(m_ui->helpAboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    // Pass filter changes to the filter model
    connect(m_ui->filterTreeWidget, &FilterTreeWidget::filterChanged, &m_orderProxyModel, &OrderSortFilterModel::setColumnFilters);

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
        QHeaderView *header = m_ui->orderTreeView->header();

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
    connect(m_ui->orderTreeView->header(), &QHeaderView::customContextMenuRequested, this, [this, createColumnMenu](const QPoint &pos)
    {
        QHeaderView *header = m_ui->orderTreeView->header();

        QMenu menu;
        createColumnMenu(&menu);

        menu.exec(header->mapToGlobal(pos));
    });

    // Update order detail buttons when sorting changes
    connect(m_ui->orderTreeView->header(), &QHeaderView::sortIndicatorChanged, this, [this]()
    {
        QItemSelectionModel *selection = m_ui->orderTreeView->selectionModel();
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
        QTreeView *tree = m_ui->orderTreeView;
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
}

void MainWindow::showSettingsDialog()
{
    SettingsDialog dlg(this);
    dlg.setCurrencies(m_shared.currencyRates.keys());
    dlg.setTargetCurrency(m_shared.targetCurrency);
    dlg.setApiKey(m_shared.apiKey);

    if (dlg.exec() == QDialog::Accepted) {
        m_shared.apiKey = dlg.apiKey();
        m_shared.targetCurrency = dlg.targetCurrency();
    }
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

    const QString description = tr("<h3>%1 v%2.%3</h3>"
                                   "Based on Qt %4 (%5)<br><br>"
                                   "Built on %6 %7<br><br>"
                                   "%8"
                                   "Copyright &copy; 2022 <a href='http://twitter.com/arturo182'>arturo182</a>. All rights reserved.<br><br>"
                                   "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, "
                                   "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.");

    QMessageBox::about(this, tr("About %1").arg(qApp->applicationName()),
                       description
                           .arg(qApp->applicationName())
                           .arg(VER_MAJOR)
                           .arg(VER_MINOR)
                           .arg(qVersion())
                           .arg(compilerVersion())
                           .arg(__DATE__)
                           .arg(__TIME__)
                       .arg(verSha1));
}

void MainWindow::openOrderWindow(const int id)
{
    OrderDetailsWidget *orderWidget = new OrderDetailsWidget(true);
    orderWidget->setAttribute(Qt::WA_DeleteOnClose);
    orderWidget->setSharedData(&m_shared);
    orderWidget->setOrder(m_orders[id]);
    orderWidget->show();
}

void MainWindow::orderTreeKeyPressEvent(QKeyEvent *event)
{
    QTreeView *tree = m_ui->orderTreeView;
    QItemSelectionModel *selection = tree->selectionModel();
    if (!selection->hasSelection())
        return;

    const int id = orderIdFromProxyModel(selection->currentIndex());
    if (id < 0)
        return;

    const Order &order= m_orders[id];

    if (event->keyCombination() == QKeyCombination(Qt::ControlModifier, Qt::Key_Return)) {
        if (!order.fulfilledAt.isValid())
            QMessageBox::information(this, tr("Not implemented"), tr("Not implemented yet, sorry"));
    } else if (event->key() == Qt::Key_Return) {
        openOrderWindow(id);
    }
}

int MainWindow::orderIdFromProxyModel(const QModelIndex &proxyIndex)
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
