#pragma once

#include <QChartView>
#include <QDate>
#include <QDialog>

namespace Ui { class StatisticsDialog; }

class OrderManager;
class SqlManager;

// Qt 5 quirk
#ifdef QT_CHARTS_USE_NAMESPACE
QT_CHARTS_USE_NAMESPACE
#endif

class StatisticsDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit StatisticsDialog(OrderManager *orderMgr, SqlManager *sqlMgr, QWidget *parent = nullptr);
        ~StatisticsDialog() override;

    public slots:
        void done(int r) override;

    private:
        void readSettings();
        void writeSettings() const;

        void showBarChart(QChartView *chartView, const QString &title, const QList<QPair<QString, int>> &data);
        void showPieChart(QChartView *chartView, const QString &title, const QList<QPair<QString, int>> &data);

        void processSales();
        void processCountries();
        void processProducts();
        void processWeight();
        void processPackaging();
        void processValue();
        void processMisc();


    private:
        OrderManager *m_orderMgr{};
        SqlManager *m_sqlMgr{};
        Ui::StatisticsDialog *m_ui;

        QPair<QDate, QDate> m_salesRange;

        QMap<QDate, int> m_ordersPerDay;
        QList<QPair<QString, int>> m_countryCounts;
        QList<QPair<QString, int>> m_productCounts;
        QList<QPair<QString, int>> m_weightCounts;
        QList<QPair<QString, int>> m_packagingCounts;
        QList<QPair<QString, int>> m_valueCounts;
};

