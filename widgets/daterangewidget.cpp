#include "daterangewidget.h"
#include "ui_daterangewidget.h"

#include <QDebug>
#include <QDate>

DateRangeWidget::DateRangeWidget(const QDate &minDate, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::DateRangeWidget)
{
    m_ui->setupUi(this);

    m_ui->fromCalendarWidget->setDateRange(minDate, QDate::currentDate());
    m_ui->toCalendarWidget->setDateRange(minDate, QDate::currentDate());

    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QWidget::close);
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, [&]()
    {
        emit rangeSelected(m_ui->fromCalendarWidget->selectedDate(), m_ui->toCalendarWidget->selectedDate());

        close();
    });
    connect(m_ui->fromCalendarWidget, &QCalendarWidget::selectionChanged, this, &DateRangeWidget::updateRangeLabel);
    connect(m_ui->toCalendarWidget,   &QCalendarWidget::selectionChanged, this, &DateRangeWidget::updateRangeLabel);
    connect(m_ui->presetListWidget, &QListWidget::currentRowChanged, this, &DateRangeWidget::applyPreset);

    setAttribute(Qt::WA_DeleteOnClose, true);
    applyPreset(1);
}

DateRangeWidget::~DateRangeWidget()
{
    delete m_ui;
}

void DateRangeWidget::setRange(const QDate &from, const QDate &to)
{
    m_ui->fromCalendarWidget->setSelectedDate(from);
    m_ui->toCalendarWidget->setSelectedDate(to);
}

void DateRangeWidget::updateRangeLabel()
{
    QDate from = m_ui->fromCalendarWidget->selectedDate();
    QDate to = m_ui->toCalendarWidget->selectedDate();

    if (from > to)
        qSwap(from, to);

    m_ui->dateRangeLabel->setText(QString("%1 - %2").arg(from.toString("MMM d, yyyy"), to.toString("MMM d, yyyy")));
}

void DateRangeWidget::applyPreset(const int idx)
{
    const QDate today = QDate::currentDate();
    const QDate monthAgo = today.addMonths(-1);

    QDate from;
    QDate to;

    switch (idx) {
    case 0: // Today
        from = today;
        to   = today;
        break;

    case 1: // Last 7 Days
        from = today.addDays(-6);
        to   = today;
        break;

    case 2: // This Month
        from = QDate(today.year(), today.month(), 1);
        to   = today;
        break;

    case 3: // Last Month
        from = QDate(monthAgo.year(), monthAgo.month(), 1);
        to   = QDate(monthAgo.year(), monthAgo.month(), monthAgo.daysInMonth());
        break;

    case 4: // Last 3 Months
    {
        const QDate threeMonthsAgo = today.addMonths(-3);
        from = QDate(threeMonthsAgo.year(), threeMonthsAgo.month(), 1);
        to   = QDate(monthAgo.year(), monthAgo.month(), monthAgo.daysInMonth());
        break;
    }

    case 5: // Last 6 Months
    {
        const QDate sixMonthsAgo = today.addMonths(-6);
        from = QDate(sixMonthsAgo.year(), sixMonthsAgo.month(), 1);
        to   = QDate(monthAgo.year(), monthAgo.month(), monthAgo.daysInMonth());
        break;
    }

    case 6: // This Year
        from = QDate(today.year(), 1, 1);
        to   = today;
        break;

    case 7: // Last Year
    {
        const QDate aYearAgo = today.addYears(-1);
        from = QDate(aYearAgo.year(), 1, 1);
        to   = QDate(aYearAgo.year(), 12, 31);
        break;
    }

    case 8: // All Time
        from = m_ui->fromCalendarWidget->minimumDate();
        to   = today;
        break;

    case 9: // Custom Range
        // from = ;
        // to   = ;
        break;
    }

    if (from.isValid() && to.isValid())
        setRange(from, to);
}
