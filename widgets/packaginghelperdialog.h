#pragma once

#include <QDialog>

namespace Ui { class PackagingHelperDialog; }

class QTreeWidgetItem;

class OrderManager;
class SqlManager;

class PackagingHelperDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit PackagingHelperDialog(OrderManager *orderMgr, SqlManager *sqlMgr, QWidget *parent = nullptr);
        ~PackagingHelperDialog() override;

        void reset();

    protected:
        void closeEvent(QCloseEvent *event) override;

    private:
        void readSettings();
        void writeSettings() const;

        void fillShippingTree();
        void fillProductTree();
        void fillOrderList();

        void processSelectionTreeCheckBox(QTreeWidgetItem *parent, QTreeWidgetItem *item, int column);
        void processProductCheckBox(QTreeWidgetItem *parent, QTreeWidgetItem *item, int column);

        void showOrderDetails(QTreeWidgetItem *current);

        void updateMatchingOrderCount();
        void updateOrderButtons();
        void updateComboLabels();
        void sortOrderList(const int idx);

        int idxFromPackagingId(const int id);

        QList<int> filteredOrders() const;

    private:
        OrderManager *m_orderMgr{};
        SqlManager *m_sqlMgr{};
        Ui::PackagingHelperDialog *m_ui{};
};
