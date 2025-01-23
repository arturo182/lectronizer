#include "orderitemdelegate.h"
#include "ordermanager.h"
#include "structs.h"

#include <QApplication>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QTextLayout>
#include <QtMath>

// Lots of code here based on qitemdelegate.cpp, Copyright (C) 2016 The Qt Company Ltd.

static QRect doCheck(const QStyleOptionViewItem &option, const QRect &bounding)
{
    QStyleOptionButton opt;
    opt.QStyleOption::operator=(option);
    opt.rect = bounding;

    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : qApp->style();

    const QRect r(opt.rect.x() + 1,
                  opt.rect.y() + 2,
                  style->pixelMetric(QStyle::PM_IndicatorWidth, &opt, widget),
                  style->pixelMetric(QStyle::PM_IndicatorHeight, &opt, widget));

    return QStyle::visualRect(opt.direction, opt.rect, r);
}

static void doLayout(const QStyleOptionViewItem &option, QRect *checkRect, QRect *textRect, bool hint)
{
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    const bool hasCheck = checkRect->isValid();
    const bool hasPixmap = false;
    const bool hasText = textRect->isValid();
    const bool hasMargin = (hasText | hasPixmap | hasCheck);
    const int frameHMargin = hasMargin ? style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1 : 0;
    const int textMargin = hasText ? frameHMargin : 0;
    const int pixmapMargin = hasPixmap ? frameHMargin : 0;
    const int checkMargin = hasCheck ? frameHMargin : 0;
    const int x = option.rect.left();
    const int y = option.rect.top();
    int w, h;

    textRect->adjust(-textMargin, 0, textMargin, 0); // add width padding
    if (textRect->height() == 0 && (!hasPixmap || !hint)) {
        //if there is no text, we still want to have a decent height for the item sizeHint and the editor size
        textRect->setHeight(option.fontMetrics.height());
    }

    QSize pm(0, 0);
    if (hint) {
        h = qMax(checkRect->height(), qMax(textRect->height(), pm.height()));
        if (option.decorationPosition == QStyleOptionViewItem::Left
            || option.decorationPosition == QStyleOptionViewItem::Right) {
            w = textRect->width() + pm.width();
        } else {
            w = qMax(textRect->width(), pm.width());
        }
    } else {
        w = option.rect.width();
        h = option.rect.height();
    }

    int cw = 0;
    QRect check;
    if (hasCheck) {
        cw = checkRect->width() + 2 * checkMargin;
        if (hint) w += cw;
        if (option.direction == Qt::RightToLeft) {
            check.setRect(x + w - cw + textMargin, y + 2, cw, h);
        } else {
            check.setRect(x + textMargin, y + 2, cw, h);
        }
    }

    // at this point w should be the *total* width

    QRect display;
    QRect decoration;
    switch (option.decorationPosition) {
    case QStyleOptionViewItem::Top: {
        if (hasPixmap)
            pm.setHeight(pm.height() + pixmapMargin); // add space
        h = hint ? textRect->height() : h - pm.height();

        if (option.direction == Qt::RightToLeft) {
            decoration.setRect(x, y, w - cw, pm.height());
            display.setRect(x, y + pm.height(), w - cw, h);
        } else {
            decoration.setRect(x + cw, y, w - cw, pm.height());
            display.setRect(x + cw, y + pm.height(), w - cw, h);
        }
        break; }
    case QStyleOptionViewItem::Bottom: {
        if (hasText)
            textRect->setHeight(textRect->height() + textMargin); // add space
        h = hint ? textRect->height() + pm.height() : h;

        if (option.direction == Qt::RightToLeft) {
            display.setRect(x, y, w - cw, textRect->height());
            decoration.setRect(x, y + textRect->height(), w - cw, h - textRect->height());
        } else {
            display.setRect(x + cw, y, w - cw, textRect->height());
            decoration.setRect(x + cw, y + textRect->height(), w - cw, h - textRect->height());
        }
        break; }
    case QStyleOptionViewItem::Left: {
        if (option.direction == Qt::LeftToRight) {
            decoration.setRect(x + cw, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        } else {
            display.setRect(x, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        }
        break; }
    case QStyleOptionViewItem::Right: {
        if (option.direction == Qt::LeftToRight) {
            display.setRect(x + cw, y, w - pm.width() - cw, h);
            decoration.setRect(display.right() + 1, y, pm.width(), h);
        } else {
            decoration.setRect(x, y, pm.width(), h);
            display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
        }
        break; }
    default:
        qWarning("doLayout: decoration position is invalid");
        break;
    }

    if (!hint) { // we only need to do the internal layout if we are going to paint
        *checkRect = QStyle::alignedRect(option.direction, Qt::AlignTop, checkRect->size(), check);
        // the text takes up all available space, unless the decoration is not shown as selected
        if (option.showDecorationSelected)
            *textRect = display;
        else
            *textRect = QStyle::alignedRect(option.direction, Qt::AlignLeft, textRect->size().boundedTo(display.size()), display);
    } else {
        *checkRect = check;
        *textRect = display;
    }
}

static QString mergeSubPair(const QPair<QString, QString> &subText)
{
    return QString("%1: %2\n").arg(subText.first).arg(subText.second);
}

OrderItemDelegate::OrderItemDelegate(OrderManager *orderMgr, QObject *parent)
    : QItemDelegate{parent}
    , m_orderMgr{orderMgr}
    , m_coloredSubText{false}
{

}

void OrderItemDelegate::setColoredSubText(const bool colored)
{
    m_coloredSubText = colored;
}

void OrderItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const int id = index.data(Qt::UserRole).toInt();
    if (!m_orderMgr->contains(id))
        return;

    const Order &order = m_orderMgr->order(id);
    const int itemIdx = index.data(Qt::UserRole + 1).toInt();
    const Item &item = order.items[itemIdx];

    const QStyleOptionViewItem opt = setOptions(index, option);

    // prepare
    painter->save();

    QRect checkRect;
    Qt::CheckState checkState = Qt::Unchecked;
    QVariant value = index.data(Qt::CheckStateRole);
    if (value.isValid()) {
        checkState = static_cast<Qt::CheckState>(value.toInt());
        checkRect = ::doCheck(opt, opt.rect);
    }

    const QString text = QString("%1x %2").arg(item.qty).arg(item.product.name);
    SubTextList subTexts;
    for (const ItemOption &itemOption : item.options)
        subTexts << qMakePair(itemOption.name, itemOption.choice);

    // do the layout
    QFont subFnt = option.font;
    subFnt.setPointSize(subFnt.pointSize() - 1);

    QRect dispRect = textRectangle(textLayoutBounds(opt, checkRect), option.font, text, subFnt, subTexts);
    ::doLayout(opt, &checkRect, &dispRect, false);

    // draw the item
    drawBackground(painter, opt, index);
    drawCheck(painter, opt, checkRect, checkState);
    drawDisplay(painter, opt, dispRect, text, subTexts);
    drawFocus(painter, opt, dispRect);

    // done
    painter->restore();
}

QSize OrderItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const int id = index.data(Qt::UserRole).toInt();
    if (!m_orderMgr->contains(id))
        return QItemDelegate::sizeHint(option, index);

    const Order &order = m_orderMgr->order(id);
    const int itemIdx = index.data(Qt::UserRole + 1).toInt();
    const Item &item = order.items[itemIdx];

    const QString text = QString("%1x %2").arg(item.qty).arg(item.product.name);

    SubTextList subTexts;
    for (const ItemOption &itemOption : item.options)
        subTexts << qMakePair(itemOption.name, itemOption.choice);

    QFont subFnt = option.font;
    subFnt.setPointSize(subFnt.pointSize() - 1);

    QRect checkRect = ::doCheck(option, option.rect);
    QRect dispRect  = textRectangle(textLayoutBounds(option, checkRect), option.font, text, subFnt, subTexts);

    ::doLayout(option, &checkRect, &dispRect, true);

    return (dispRect | checkRect).size();
}

QSizeF OrderItemDelegate::doTextLayout(const int lineWidth) const
{
    qreal height = 0;
    qreal widthUsed = 0;

    m_textLayout.beginLayout();

    while (true) {
        QTextLine line = m_textLayout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));

        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }

    m_textLayout.endLayout();

    return QSizeF(widthUsed, height);
}

QRect OrderItemDelegate::textLayoutBounds(const QStyleOptionViewItem &option, const QRect &checkRect) const
{
#define QFIXED_MAX (INT_MAX/256)

    QRect rect = option.rect;
    const QWidget *w = option.widget;
    QStyle *style = w ? w->style() : QApplication::style();
    const bool wrapText = option.features & QStyleOptionViewItem::WrapText;

    // see QItemDelegate::drawDisplay
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, w) + 1;

    switch (option.decorationPosition) {
    case QStyleOptionViewItem::Left:
    case QStyleOptionViewItem::Right:
        rect.setWidth(wrapText && rect.isValid() ? rect.width() - 2 * textMargin : (QFIXED_MAX));
        break;

    case QStyleOptionViewItem::Top:
    case QStyleOptionViewItem::Bottom:
        rect.setWidth(wrapText ? option.decorationSize.width() - 2 * textMargin : (QFIXED_MAX));
        break;
    }

    if (wrapText) {
        if (!checkRect.isNull())
            rect.setWidth(rect.width() - checkRect.width() - 2 * textMargin);

        // adjust height to be sure that the text fits
        const QSizeF size = doTextLayout(rect.width());
        rect.setHeight(qCeil(size.height()));
    }

    return rect;
}

QRect OrderItemDelegate::textRectangle(const QRect &rect, const QFont &font, const QString &text, const QFont &subFont, const SubTextList &subTexts) const
{
    m_textOption.setWrapMode(QTextOption::WordWrap);
    m_textLayout.setTextOption(m_textOption);

    m_textLayout.setFont(font);
    m_textLayout.setText(text);

    const QSizeF fSize = doTextLayout(rect.width());
    const QSize size(qCeil(fSize.width()), qCeil(fSize.height()));

    QSize subSize;
    for (const auto &subPair : subTexts) {
        m_textLayout.setFont(subFont);
        m_textLayout.setText(mergeSubPair(subPair));

        const QSizeF subFSize = doTextLayout(rect.width());
        subSize = { qMax(subSize.width(), qCeil(subFSize.width())), subSize.height() + qCeil(subFSize.height()) };
    }

    const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr) + 1;
    return QRect(0, 0, size.width() + 2 * textMargin, size.height() + subSize.height() + 2);
}

void OrderItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text, const SubTextList &subTexts) const
{
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight));
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    if (text.isEmpty())
        return;

    if (option.state & QStyle::State_Editing) {
        painter->save();
        painter->setPen(option.palette.color(cg, QPalette::Text));
        painter->drawRect(rect.adjusted(0, 0, -1, -1));
        painter->restore();
    }

    const QStyleOptionViewItem opt = option;

    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, widget) + 1;
    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = opt.features & QStyleOptionViewItem::WrapText;
    m_textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);

    m_textOption.setTextDirection(option.direction);
    m_textOption.setAlignment(QStyle::visualAlignment(option.direction, Qt::AlignLeft));

    m_textLayout.setTextOption(m_textOption);
    m_textLayout.setFont(option.font);
    m_textLayout.setText(text);

    QSizeF textLayoutSize = doTextLayout(textRect.width());

    if (textRect.width() < textLayoutSize.width() || textRect.height() < textLayoutSize.height()) {
        QString elided;
        int start = 0;
        int end = text.indexOf(QChar::LineSeparator, start);
        if (end == -1) {
            elided += option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
        } else {
            while (end != -1) {
                elided += option.fontMetrics.elidedText(text.mid(start, end - start), option.textElideMode, textRect.width());
                elided += QChar::LineSeparator;
                start = end + 1;
                end = text.indexOf(QChar::LineSeparator, start);
            }
            //let's add the last line (after the last QChar::LineSeparator)
            elided += option.fontMetrics.elidedText(text.mid(start),
                                                    option.textElideMode, textRect.width());
        }
        m_textLayout.setText(elided);
        textLayoutSize = doTextLayout(textRect.width());
    }

    const QSize layoutSize(textRect.width(), int(textLayoutSize.height()));
    QRect layoutRect = QStyle::alignedRect(option.direction, /*option.displayAlignment*/Qt::AlignLeft, layoutSize, textRect);
    // if we still overflow even after eliding the text, enable clipping
    if (textRect.width() < textLayoutSize.width() || textRect.height() < textLayoutSize.height()) {
        painter->save();
        painter->setClipRect(layoutRect);
        m_textLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
        painter->restore();
    } else {
        m_textLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
    }

    textRect.adjust(textMargin * 2, 0, 0, 0);


    if (!(option.state & QStyle::State_Selected))
        painter->setPen(option.palette.color(cg, QPalette::Shadow));

    for (const auto &subPair : subTexts) {
        textRect.adjust(0, layoutRect.height() - 1, 0, 0);

        const QString subText = mergeSubPair(subPair);
        QFont subFont = option.font;
        subFont.setPointSize(subFont.pointSize() - 1);
        m_textLayout.setFont(subFont);
        m_textLayout.setText(subText);

        QSizeF subTextLayoutSize = doTextLayout(textRect.width());

        if (textRect.width() < subTextLayoutSize.width() || textRect.height() < subTextLayoutSize.height()) {
            QString elided;
            int start = 0;
            int end = subText.indexOf(QChar::LineSeparator, start);
            if (end == -1) {
                elided += option.fontMetrics.elidedText(subText, option.textElideMode, textRect.width());
            } else {
                while (end != -1) {
                    elided += option.fontMetrics.elidedText(subText.mid(start, end - start), option.textElideMode, textRect.width());
                    elided += QChar::LineSeparator;
                    start = end + 1;
                    end = subText.indexOf(QChar::LineSeparator, start);
                }

                //let's add the last line (after the last QChar::LineSeparator)
                elided += option.fontMetrics.elidedText(subText.mid(start), option.textElideMode, textRect.width());
            }

            m_textLayout.setText(elided);
            subTextLayoutSize = doTextLayout(textRect.width());
        }

        QVector<QTextLayout::FormatRange> formatRange;

        if (m_coloredSubText && m_textLayout.text().endsWith("\n")) {
            const QColor baseColor = painter->pen().color();

            QTextCharFormat fmt;
            int len = 0;

            if (subPair.second.toLower() == tr("yes")) {
                len = tr("yes").length();
                fmt.setForeground(baseColor.lightness() < 128 ? QColor(0, 150, 0) : QColor(100, 255, 100));
            } else if (subPair.second.toLower() == tr("no")) {
                len = tr("no").length();
                fmt.setForeground(baseColor.lightness() < 128 ? QColor(200, 0, 0) : QColor(255, 100, 100));
            } else {
                len = subPair.second.length();
                fmt.setForeground(baseColor.lightness() < 128 ? QColor(200, 100, 0) : QColor(255, 165, 50));
            }

            if (len > 0)
                formatRange << QTextLayout::FormatRange{ (int)m_textLayout.text().length() - len - 1, len, fmt };
        }

        const QSize subLayoutSize(textRect.width(), int(subTextLayoutSize.height()));
        layoutRect = QStyle::alignedRect(option.direction, Qt::AlignLeft, subLayoutSize, textRect);
        // if we still overflow even after eliding the text, enable clipping
        if (textRect.width() < subTextLayoutSize.width() || textRect.height() < subTextLayoutSize.height()) {
            painter->save();
            painter->setClipRect(layoutRect);
            m_textLayout.draw(painter, layoutRect.topLeft(), formatRange, layoutRect);
            painter->restore();
        } else {
            m_textLayout.draw(painter, layoutRect.topLeft(), formatRange, layoutRect);
        }
    }
}

bool OrderItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_ASSERT(event);
    Q_ASSERT(model);

    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable) || !(option.state & QStyle::State_Enabled) || !(flags & Qt::ItemIsEnabled))
        return false;

    // make sure that we have a check state
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return false;

    // make sure that we have the right event type
    if ((event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseButtonDblClick) || (event->type() == QEvent::MouseButtonPress)) {
        QRect checkRect = ::doCheck(option, option.rect);
        QRect emptyRect;

        ::doLayout(option, &checkRect, &emptyRect, false);

        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton || !checkRect.contains(me->pos()))
            return false;

        // eat the double click events inside the check rect
        if ((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonDblClick))
            return true;

    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
    } else {
        return false;
    }

    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
    if (flags & Qt::ItemIsUserTristate)
        state = ((Qt::CheckState)((state + 1) % 3));
    else
        state = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;

    return model->setData(index, state, Qt::CheckStateRole);
}
