#include "filterbuttondelegate.h"

#include <QPainter>
#include <QTreeWidget>

FilterButtonDelegate::FilterButtonDelegate(QTreeWidget *treeWidget)
    : QItemDelegate(treeWidget)
    , m_treeWidget(treeWidget)
{

}

void FilterButtonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QAbstractItemModel *model = index.model();

    // Draw the sub tree widgets normally
    if (model->parent(index).isValid()) {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionButton buttonOption;
    buttonOption.features = QStyleOptionButton::None;
    buttonOption.palette  = option.palette;
    buttonOption.rect     = option.rect;
    buttonOption.state    = option.state & ~QStyle::State_HasFocus;

    painter->save();

    const QBrush buttonBrush = option.palette.button();
    QColor buttonColor(230, 230, 230);
    if (!buttonBrush.gradient() && buttonBrush.texture().isNull())
        buttonColor = buttonBrush.color();

    const QColor outlineColor = buttonColor.darker(150);
    const QColor highlightColor = buttonColor.lighter(130);

    // Only draw topline if the previous item is expanded
    const QModelIndex previousIndex = model->index(index.row() - 1, index.column());
    const bool drawTopline = (index.row() > 0 && m_treeWidget->isExpanded(previousIndex));
    const int highlightOffset = drawTopline ? 1 : 0;

    QLinearGradient gradient(option.rect.topLeft(), option.rect.bottomLeft());
    gradient.setColorAt(0, buttonColor.lighter(102));
    gradient.setColorAt(1, buttonColor.darker(106));

    // Frame
    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawRect(option.rect);
    painter->setPen(highlightColor);
    painter->drawLine(option.rect.topLeft() + QPoint(0, highlightOffset), option.rect.topRight() + QPoint(0, highlightOffset));
    painter->setPen(outlineColor);
    if (drawTopline)
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    painter->restore();

    const int i = 9; // ### hardcoded in qcommonstyle.cpp
    const QRect r = option.rect;

    // Indicator
    QStyleOption branchOption;
    branchOption.palette = option.palette;
    branchOption.rect    = QRect(r.left() + i/2, r.top() + (r.height() - i)/2, i, i);
    branchOption.state   = QStyle::State_Children;

    if (m_treeWidget->isExpanded(index))
        branchOption.state |= QStyle::State_Open;

    m_treeWidget->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, painter, m_treeWidget);

    // Text
    const QString text = option.fontMetrics.elidedText(model->data(index, Qt::DisplayRole).toString(), Qt::ElideMiddle, r.width());
    m_treeWidget->style()->drawItemText(painter, r, Qt::AlignCenter, option.palette, m_treeWidget->isEnabled(), text);
}

QSize FilterButtonDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    return QItemDelegate::sizeHint(opt, index) + QSize(2, 2);
}
