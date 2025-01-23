#pragma once

#include <QWidget>

namespace Ui
{
    class DateRangeWidget;
}

class DateRangeWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit DateRangeWidget(const QDate &minDate, QWidget *parent = nullptr);
        ~DateRangeWidget() override;

        void setRange(const QDate &from, const QDate &to);

    signals:
        void rangeSelected(const QDate &from, const QDate &to);

    private:
        void updateRangeLabel();
        void applyPreset(const int idx);

    private:
        Ui::DateRangeWidget *m_ui;
};

