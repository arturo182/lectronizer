#pragma once

#include <QDialog>

namespace Ui { class StatisticsDialog; }

class QChartView;

class OrderManager;
class SqlManager;

class StatisticsDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit StatisticsDialog(OrderManager *orderMgr, SqlManager *sqlMgr, QWidget *parent = nullptr);
        ~StatisticsDialog();

    private:
        void showBarChart(QChartView *chartView, const QString &title, const QList<QPair<QString, int>> &data);
        void showPieChart(QChartView *chartView, const QString &title, const QList<QPair<QString, int>> &data);

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

        QList<QPair<QString, int>> m_countryCounts;
        QList<QPair<QString, int>> m_productCounts;
        QList<QPair<QString, int>> m_weightCounts;
        QList<QPair<QString, int>> m_packagingCounts;
        QList<QPair<QString, int>> m_valueCounts;
};

