#include "widgets/daterangewidget.h"
#include "ordermanager.h"
#include "statisticsdialog.h"
#include "sqlmanager.h"
#include "ui_statisticsdialog.h"

#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QDateTimeAxis>
#include <QLineSeries>
#include <QPieSeries>
#include <QProgressDialog>
#include <QSettings>
#include <QSplineSeries>
#include <QValueAxis>

// Qt 5 quirk
#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif

StatisticsDialog::StatisticsDialog(OrderManager *orderMgr, SqlManager *sqlMgr, QWidget *parent)
    : QDialog{parent}
    , m_orderMgr{orderMgr}
    , m_sqlMgr{sqlMgr}
    , m_ui{new Ui::StatisticsDialog}
{
    m_ui->setupUi(this);

    QProgressDialog progressDlg(tr("Gathering data"), tr("Cancel"), 0, 100, this);
    progressDlg.show();

    // pretty way to add new tabs and make the progress dialog % follow
    const QList<std::function<void()>> funcs =
    {
        std::bind(&StatisticsDialog::processSales, this),
        std::bind(&StatisticsDialog::processCountries, this),
        std::bind(&StatisticsDialog::processProducts, this),
        std::bind(&StatisticsDialog::processWeight, this),
        std::bind(&StatisticsDialog::processPackaging, this),
        std::bind(&StatisticsDialog::processValue, this),
        std::bind(&StatisticsDialog::processMisc, this),
    };

    const int step = (100 / funcs.size()) + 1;

    for (int i = 0; i < funcs.size(); ++i) {
        funcs[i]();
        progressDlg.setValue(step * (i + 1));
        qApp->processEvents();

    }

    readSettings();
}

StatisticsDialog::~StatisticsDialog()
{
    delete m_ui;
    m_ui = nullptr;
}

void StatisticsDialog::done(int r)
{
    writeSettings();

    QDialog::done(r);
}

void StatisticsDialog::readSettings()
{
    QSettings set;

    set.beginGroup("StatisticsDialog");
    restoreGeometry(set.value("geometry").toByteArray());
    setWindowState(Qt::WindowStates(set.value("state").toInt()));
}

void StatisticsDialog::writeSettings() const
{
    QSettings set;

    set.beginGroup("StatisticsDialog");
    set.setValue("geometry", saveGeometry());
    set.setValue("state", (int)windowState());
}

void StatisticsDialog::showBarChart(QChartView *chartView, const QString &title, const QList<QPair<QString, int>> &data)
{
    QStringList keyList;
    for (const auto &pair : data)
        keyList << pair.first;

    int maxValue = 1;
    QList<qreal> valueList;
    for (const auto &pair : data) {
        valueList << pair.second;

        maxValue = qMax(maxValue, pair.second);
    }

    // key axis
    QBarCategoryAxis *xAxis = new QBarCategoryAxis();
    xAxis->append(keyList);
    xAxis->setLabelsAngle(270);

    // value axis
    QValueAxis *yAxis = new QValueAxis();
    yAxis->setRange(0, maxValue * 1.25);
    yAxis->setLabelFormat("%d");

    // chart
    QChart *chart = new QChart();
    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yAxis, Qt::AlignLeft);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setBackgroundRoundness(0);
    chart->setTitle(title);
    QFont titleFont = chart->titleFont();
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);
    chart->legend()->setVisible(false);

    // series
    for (const auto &pair : data) {
        QBarSeries *series = new QBarSeries();
        series->setLabelsVisible(true);
        series->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);

        QBarSet *orderSet = new QBarSet(pair.first);
        orderSet->append(pair.second);
        orderSet->setLabelColor(Qt::black);
        series->append(orderSet);

        chart->addSeries(series);

        series->attachAxis(yAxis);
    }

    // set chart
    QChart *previousChart = chartView->chart();

    chartView->setChart(chart);

    delete previousChart;
}

void StatisticsDialog::showPieChart(QChartView *chartView, const QString &title, const QList<QPair<QString, int>> &data)
{
    QStringList keyList;
    for (const auto &pair : data)
        keyList << pair.first;

    QList<qreal> valueList;
    for (const auto &pair : data)
        valueList << pair.second;


    // series
    QPieSeries *series = new QPieSeries();
    for (const auto &pair : data)
        series->append(pair.first, pair.second);

    for (QPieSlice *slice : series->slices()) {
        if (slice->percentage() < 0.02)
            continue;

        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelInsideNormal);
    }

    // chart
    QChart *chart = new QChart();
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setBackgroundRoundness(0);
    chart->setTitle(title);
    QFont titleFont = chart->titleFont();
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->addSeries(series);

    // set chart
    QChart *previousChart = chartView->chart();

    chartView->setChart(chart);

    delete previousChart;
}

void StatisticsDialog::processSales()
{
    const auto updateChartRange = [&](const QDate &from, const QDate &to)
    {
        QLineSeries *series = new QLineSeries();
        int totalOrders = 0;

        QMapIterator<QDate, int> it(m_ordersPerDay);
        while (it.hasNext()) {
            it.next();

            if (it.key() >= from && it.key() <= to) {
                series->append(QDateTime(it.key(), QTime()).toMSecsSinceEpoch(), it.value());
                totalOrders += it.value();
            }
        }

        QString format = "MMM d, yyyy";
        QString xTitle = QString("Days (%1 total)").arg(series->count());
        int tickDiv = 1;
        if (series->count() > 60) {
            tickDiv = 30;
        } else if (series->count() > 365) {
            tickDiv = 60;
        } else if (series->count() > 365 * 2) {
            tickDiv = 90;
        }

        if (tickDiv != 1) {
            format = "MMM yyyy";
            xTitle = QString("Months (%1 days total)").arg(series->count());
        }

        QChart *chart = new QChart();
        chart->addSeries(series);
        chart->legend()->hide();

        QDateTimeAxis *xAxis = new QDateTimeAxis();
        xAxis->setTickCount(series->count() / tickDiv);
        xAxis->setFormat(format);
        xAxis->setLabelsAngle(270);
        xAxis->setTitleText(xTitle);
        chart->addAxis(xAxis, Qt::AlignBottom);
        series->attachAxis(xAxis);

        QValueAxis *yAxis = new QValueAxis;
        yAxis->setLabelFormat("%i");
        yAxis->setTitleText(QString("Orders (%1 total)").arg(totalOrders));
        chart->addAxis(yAxis, Qt::AlignLeft);
        series->attachAxis(yAxis);

        QChart *previousChart = m_ui->salesChartView->chart();

        m_ui->salesChartView->setChart(chart);

        delete previousChart;

        m_ui->salesDateRangeLabel->setText(QString("%1 to %2").arg(from.toString("MMM d, yyyy"), to.toString("MMM d, yyyy")));

        m_salesRange = qMakePair(from, to);
    };

    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);
        const QDate date = order.createdAt.date();

        if (m_ordersPerDay.contains(date)) {
            m_ordersPerDay.insert(date, m_ordersPerDay.value(date) + 1);
        } else {
            m_ordersPerDay.insert(date, 1);
        }
    }

    const QDate lastDate = m_ordersPerDay.lastKey();

    QDate currentDate = m_ordersPerDay.firstKey();
    while (currentDate <= lastDate) {
        if (!m_ordersPerDay.contains(currentDate)) {
            m_ordersPerDay.insert(currentDate, 0);
        }
        currentDate = currentDate.addDays(1);
    }

    connect(m_ui->salesChangeButton, &QPushButton::clicked, this, [&, updateChartRange]()
    {
        const QPoint buttonPos = m_ui->salesChangeButton->mapToGlobal(QPoint(0, m_ui->salesChangeButton->height()));

        DateRangeWidget *dateRange = new DateRangeWidget(m_ordersPerDay.firstKey(), this);
        dateRange->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
        dateRange->move(dateRange->mapFromGlobal(buttonPos));
        dateRange->setRange(m_salesRange.first, m_salesRange.second);
        dateRange->show();

        connect(dateRange, &DateRangeWidget::rangeSelected, this, updateChartRange);
    });

    // last 7 days
    m_salesRange = qMakePair(QDate::currentDate().addDays(-6), QDate::currentDate());
    updateChartRange(m_salesRange.first, m_salesRange.second);
}

void StatisticsDialog::processCountries()
{
    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);
        const QString country = order.shipping.address.country;

        bool exists = false;
        for (auto &pair : m_countryCounts) {
            if (pair.first != country)
                continue;

            pair.second += 1;

            exists = true;
            break;
        }

        if (!exists)
            m_countryCounts << qMakePair(country, 1);
    }

    std::sort(m_countryCounts.begin(), m_countryCounts.end(), [](const auto &left, const auto &right)
    {
        return left.second >= right.second;
    });

    connect(m_ui->countriesTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [&](const int &idx)
    {
        const QString title = tr("Orders per country");

        if (idx == 0)  {
            showBarChart(m_ui->countriesChartView, title, m_countryCounts);
        } else {
            showPieChart(m_ui->countriesChartView, title, m_countryCounts);
        }
    });
    emit m_ui->countriesTypeCombo->currentIndexChanged(0);
}

void StatisticsDialog::processProducts()
{
    struct ProductKey { QString name; QString sku; };
    QList<QPair<ProductKey, int>> productList;

    // first iterate products and skip options
    // products are guaranteed to have at least a name, and maybe a sku
    // this will give us a good starting point on a product list
    // and allow us to match names to skus in options
    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);

        for (const Item &item : order.items) {
            const QString name = item.product.name;
            const QString sku = item.product.sku;

            if (sku.isEmpty())
                continue;

            bool exists = false;
            for (auto &key : productList) {
                if ((name == key.first.name) || (sku == key.first.sku)) {

                    // if some orders have been made before sku was added,
                    // add the sku to the category from later orders
                    if (key.first.sku.isEmpty())
                        key.first.sku = sku;

                    if (key.first.name.isEmpty())
                        key.first.name = name;

                    key.second += item.qty;

                    exists = true;
                    break;
                }
            }

            if (!exists)
                productList << qMakePair<ProductKey, int>({ name, sku }, (int)item.qty);
        }
    }

    // now iterate options, we only add options that have skus
    // we'll match the product list based on skus
    // because names of options might not match names of products
    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);
        for (const Item &item : order.items) {
            for (const ItemOption &option : item.options) {
                const QString name = option.name + ": " + option.choice;
                const QString sku = option.sku;

                if (sku.isEmpty())
                    continue;

                bool exists = false;
                for (auto &key : productList) {
                    if (key.first.sku != sku)
                        continue;

                    if (key.first.name.isEmpty())
                        key.first.name = name;

                    key.second += 1;
                    exists = true;
                }

                // TODO: once we have a product manager, try to grab the name based on sku from there
                if (!exists)
                    productList << qMakePair<ProductKey, int>({ name, sku }, 1);
            }
        }
    }

    // normalize the keys
    for (const auto &product : productList) {
        const int fullLength = product.first.name.length();
        QString name = product.first.name.left(25);

        if (name.length() < fullLength)
            name += "...";

        if (!product.first.sku.isEmpty()) {
            if (name.isEmpty()) {
                name = tr("SKU: ") + product.first.sku;
            } else {
                name += tr("<br>(SKU: %1)").arg(product.first.sku);
            }
        }

        m_productCounts << qMakePair(name, product.second);
    }

    std::sort(m_productCounts.begin(), m_productCounts.end(), [](const auto &left, const auto &right)
    {
        return left.second >= right.second;
    });

    connect(m_ui->productsTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [&](const int &idx)
    {
        const QString title = tr("Total quantity sold per product");

        if (idx == 0)  {
            showBarChart(m_ui->productsChartView, title, m_productCounts);
        } else {
            showPieChart(m_ui->productsChartView, title, m_productCounts);
        }
    });
    emit m_ui->productsTypeCombo->currentIndexChanged(0);
}

void StatisticsDialog::processWeight()
{
    QList<QPair<int, int>> buckets;
    QString unit = "gr";

    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);
        const int weight = order.weight.total;
        const int weightBucket = (weight / 10) * 10;

        unit = order.weight.unit;

        bool exists = false;
        for (auto &pair : buckets) {
            if (pair.first != weightBucket)
                continue;

            pair.second += 1;

            exists = true;
            break;
        }

        if (!exists)
            buckets << qMakePair(weightBucket, 1);
    }

    std::sort(buckets.begin(), buckets.end(), [](const auto &left, const auto &right)
    {
        return left.first <= right.first;
    });

    for (const auto &pair : buckets)
        m_weightCounts << qMakePair(QString("%1-%2%3").arg(pair.first).arg(pair.first + 9).arg(unit), pair.second);

    connect(m_ui->weightTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [&](const int &idx)
    {
        const QString title = tr("Order weight distribution");

        if (idx == 0)  {
            showBarChart(m_ui->weightChartView, title, m_weightCounts);
        } else {
            showPieChart(m_ui->weightChartView, title, m_weightCounts);
        }
    });
    emit m_ui->weightTypeCombo->currentIndexChanged(0);
}

void StatisticsDialog::processPackaging()
{
    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);
        QString packaging = tr("Default packaging");

        if (order.packaging < 0)
            continue;

        for (const Packaging &pack : m_sqlMgr->packagings()) {
            if (pack.id != order.packaging)
                continue;

            packaging = pack.name;
        }

        bool exists = false;
        for (auto &pair : m_packagingCounts) {
            if (pair.first != packaging)
                continue;

            pair.second += 1;

            exists = true;
            break;
        }

        if (!exists)
            m_packagingCounts << qMakePair(packaging, 1);
    }

    std::sort(m_packagingCounts.begin(), m_packagingCounts.end(), [](const auto &left, const auto &right)
    {
        return left.second >= right.second;
    });

    connect(m_ui->packagingTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [&](const int &idx)
    {
        const QString title = tr("Orders per packaging");

        if (idx == 0)  {
            showBarChart(m_ui->packagingChartView, title, m_packagingCounts);
        } else {
            showPieChart(m_ui->packagingChartView, title, m_packagingCounts);
        }
    });
    emit m_ui->packagingTypeCombo->currentIndexChanged(0);
}

void StatisticsDialog::processValue()
{
    QList<QPair<int, int>> buckets;
    QString currency = "EUR";

    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);
        const int value = order.total;
        const int valueBucket = (value / 10) * 10;

        currency = order.currency;

        bool exists = false;
        for (auto &pair : buckets) {
            if (pair.first != valueBucket)
                continue;

            pair.second += 1;

            exists = true;
            break;
        }

        if (!exists)
            buckets << qMakePair(valueBucket, 1);
    }

    std::sort(buckets.begin(), buckets.end(), [](const auto &left, const auto &right)
    {
        return left.first <= right.first;
    });

    for (const auto &pair : buckets)
        m_valueCounts << qMakePair(QString("%1-%2 %3").arg(pair.first).arg(pair.first + 9).arg(currency), pair.second);

    connect(m_ui->valueTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [&](const int &idx)
    {
        const QString title = tr("Order value distribution");

        if (idx == 0)  {
            showBarChart(m_ui->valueChartView, title, m_valueCounts);
        } else {
            showPieChart(m_ui->valueChartView, title, m_valueCounts);
        }
    });
    emit m_ui->valueTypeCombo->currentIndexChanged(0);
}

void StatisticsDialog::processMisc()
{
    // for each of {value, weight, ship time}, we keep min and max value (stats)
    // and we keep the order id for those stats (orders)
    // finally, update the average sum so later we get the average

    QPair<double, double> valueStats{INT_MAX, INT_MIN};
    QPair<int, int> valueOrders{0, 0};
    double valueAvg = 0;
    double valueTotal = 0;
    QString valueCurrency = "EUR";

    QPair<double, double> weightStats{INT_MAX, INT_MIN};
    QPair<int, int> weightOrders{0, 0};
    double weightAvg = 0;
    QString weightUnit = "gr";

    QPair<int, int> shipTimeStats{INT_MAX, INT_MIN}; // seconds
    QPair<int, int> shipTimeOrders{0, 0};
    int shipTimeAvg = 0; // seconds

    int orderCount = m_orderMgr->orderIds().size();
    int shippedOrderCount = 0;
    int refundedOrderCount = 0;

    const auto processOrder = [](const int orderId, const auto value, auto &stats, auto &orders, auto &avg)
    {
        // update min
        if (value < stats.first) {
            stats.first = value;
            orders.first = orderId;
        }

        // update max
        if (value > stats.second) {
            stats.second = value;
            orders.second = orderId;
        }

        avg += value;
    };

    const auto prettySeconds = [](const int totalSecs) -> QString
    {
        const int secsPerMin  = 60;
        const int minPerHour  = 60;
        const int hoursPerDay = 24;
        const int secsPerDay  = (secsPerMin * minPerHour * hoursPerDay);
        const int secsPerHour = (secsPerMin * minPerHour);

        const int days    = totalSecs / secsPerDay;
        const int hours   = (totalSecs - days * secsPerDay) / secsPerHour;
        const int seconds = totalSecs % secsPerMin;
        const int minutes = (totalSecs - seconds) / secsPerMin % minPerHour;

        QString result;
        if (days > 0)
            result += tr("%1 days ").arg(days);

        if ((hours > 0) || (days > 0))
            result += tr("%1 hours ").arg(hours);

        if ((minutes > 0) || (hours > 0) || (days > 0))
            result += tr("%1 min ").arg(minutes);

        result += tr("%1 sec").arg(seconds);

        return result;
    };

    for (const int id : m_orderMgr->orderIds()) {
        const Order &order = m_orderMgr->order(id);

        valueCurrency = order.currency;
        weightUnit = order.weight.unit;

        valueTotal += order.total;

        processOrder(id, order.total, valueStats, valueOrders, valueAvg);
        processOrder(id, order.calcWeight(), weightStats, weightOrders, weightAvg);

        if (order.isShipped() && !order.isRefunded()) {
            processOrder(id, order.createdAt.secsTo(order.fulfilledAt), shipTimeStats, shipTimeOrders, shipTimeAvg);

            shippedOrderCount += 1;
        } else if (order.isRefunded()) {
            refundedOrderCount += 1;
        }
    }

    valueAvg    /= orderCount;
    weightAvg   /= orderCount;
    if (shippedOrderCount > 0)
        shipTimeAvg /= shippedOrderCount;

    const auto orderLink = [&](const int id) -> QString
    {
        const Order &order = m_orderMgr->order(id);
        return tr("<a href='%1'>#%2</a>").arg(order.editUrl(), QString::number(order.id));
    };

    m_ui->miscValueMinLabel->setText(tr("%1 %2 (Order %3)").arg(valueStats.first).arg(valueCurrency).arg(orderLink(valueOrders.first)));
    m_ui->miscValueMaxLabel->setText(tr("%1 %2 (Order %3)").arg(valueStats.second).arg(valueCurrency).arg(orderLink(valueOrders.second)));
    m_ui->miscValueAvgLabel->setText(tr("%1 %2").arg(valueAvg, 0, 'f', 2).arg(valueCurrency));
    m_ui->miscValueTotalLabel->setText(tr("%1 %2").arg(valueTotal, 0, 'f', 2).arg(valueCurrency));

    m_ui->miscWeightMinLabel->setText(tr("%1 %2 (Order %3)").arg(weightStats.first).arg(weightUnit).arg(orderLink(weightOrders.first)));
    m_ui->miscWeightMaxLabel->setText(tr("%1 %2 (Order %3)").arg(weightStats.second).arg(weightUnit).arg(orderLink(weightOrders.second)));
    m_ui->miscWeightAvgLabel->setText(tr("%1 %2").arg(weightAvg, 0, 'f', 2).arg(weightUnit));

    m_ui->miscShipTimeMinLabel->setText(tr("%1 (Order %3)").arg(prettySeconds(shipTimeStats.first)).arg(orderLink(shipTimeOrders.first)));
    m_ui->miscShipTimeMaxLabel->setText(tr("%1 (Order %3)").arg(prettySeconds(shipTimeStats.second)).arg(orderLink(shipTimeOrders.second)));
    m_ui->miscShipTimeAvgLabel->setText(prettySeconds(shipTimeAvg));

    m_ui->miscInfoLabel->setText(tr("Statistics based on %1 orders (%2 shipped, excluding %3 refunded).").arg(orderCount).arg(shippedOrderCount).arg(refundedOrderCount));
}
