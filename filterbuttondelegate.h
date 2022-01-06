#pragma once

#include <QItemDelegate>

class QTreeWidget;

// Based on qttools SheetDelegate class, copyright (C) 2016 The Qt Company Ltd.
class FilterButtonDelegate: public QItemDelegate
{
    Q_OBJECT

    public:
        explicit FilterButtonDelegate(QTreeWidget *treeWidget);

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
        QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const override;

    private:
        QTreeWidget *m_treeWidget{};
};
