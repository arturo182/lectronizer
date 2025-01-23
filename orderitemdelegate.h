#pragma once

#include <QItemDelegate>
#include <QTextLayout>
#include <QTextOption>

class OrderManager;

class OrderItemDelegate : public QItemDelegate
{
    Q_OBJECT

    typedef QList<QPair<QString, QString>> SubTextList;

    public:
        explicit OrderItemDelegate(OrderManager *orderMgr, QObject *parent = nullptr);

        void setColoredSubText(const bool colored);

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    private:
        QSizeF doTextLayout(const int lineWidth) const;
        QRect textLayoutBounds(const QStyleOptionViewItem &option, const QRect &checkRect) const;
        QRect textRectangle(const QRect &rect, const QFont &font, const QString &text, const QFont &subFont, const SubTextList &subTexts) const;
        void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text, const SubTextList &subTexts) const;
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    private:
        OrderManager *m_orderMgr{};
        mutable QTextOption m_textOption{};
        mutable QTextLayout m_textLayout{};
        bool m_coloredSubText{};
};
