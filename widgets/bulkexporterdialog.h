#pragma once

#include <QDialog>

namespace Ui { class BulkExporterDialog; }

class OrderManager;
class QSortFilterProxyModel;
class SqlManager;
struct SharedData;

enum class ColumnId;

class BulkExporterDialog : public QDialog
{
    Q_OBJECT

    public:
        enum Separator
        {
            SepComma = 0,
            SepSemicolon,
            SepTab
        };

    public:
        explicit BulkExporterDialog(OrderManager *orderMgr, QSortFilterProxyModel *proxyModel, SqlManager *sqlMgr, SharedData *shared, QWidget *parent = nullptr);
        ~BulkExporterDialog() override;

    public slots:
        void done(int r) override;

    protected:
        void closeEvent(QCloseEvent *event) override;

    private:
        void readSettings();
        void writeSettings() const;
        void connectSignals();

        void addPreset(const QString &name, const QList<ColumnId> &columns);
        void applyPreset(const QList<ColumnId> &columns);

        void updateCurrentPreset();

        void updatePreview();
        QString generateCsv(const bool preview = false);

        QList<ColumnId> currentColumnList() const;
        QVariant valueForOrderColumn(const int id, const ColumnId &column) const;

    private slots:
        void updateColumnButtons();

    private:
        QString m_lastSaveDir{};
        QList<int> m_orderIds{};
        OrderManager *m_orderMgr{};
        QSortFilterProxyModel *m_proxyModel{};
        SharedData *m_shared{};
        SqlManager *m_sqlMgr{};
        Ui::BulkExporterDialog *m_ui;
};

