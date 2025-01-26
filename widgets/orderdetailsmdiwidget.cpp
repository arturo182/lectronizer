#include "orderdetailsmdiwidget.h"
#include "orderdetailswidget.h"
#include "ui_orderdetailsmdiwidget.h"

#include <QCloseEvent>
#include <QtMath>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleOptionTab>

OrderDetailsMdiWidget *OrderDetailsMdiWidget::instance = nullptr;

// based on QCommonStyle, Copyright (C) 2016 The Qt Company Ltd.
class HorizontalTabStyle : public QProxyStyle
{
    public:
        QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const override
        {
            QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
            if (type == QStyle::CT_TabBarTab) {
                if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                    const QStringList lines = tab->text.split("\n", Qt::SkipEmptyParts);
#else
                    const QStringList lines = tab->text.split("\n", QString::SkipEmptyParts);
#endif
                    const QFontMetrics fm = option->fontMetrics;

                    int textWidth = 0;
                    int textHeight = 0;
                    for (const QString &line : lines) {
                        textWidth = qMax(textWidth, tab->fontMetrics.horizontalAdvance(line));
                        textHeight += fm.boundingRect(line).height();
                    }

                    const int hframe = pixelMetric(QStyle::PM_TabBarTabHSpace, option, widget);
                    const int vframe = pixelMetric(QStyle::PM_TabBarTabVSpace, option, widget);
                    const int padding = 4;


                    return QSize(hframe + textWidth + tab->rightButtonSize.width() + padding, vframe + textHeight);
                }
            }
            return s;
        }

        QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const override
        {
            if ((element == SE_TabBarTabRightButton) || (element == SE_TabBarTabLeftButton)) {
                QRect r;
                if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                    const bool selected = tab->state & State_Selected;
                    const int verticalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftVertical, tab, widget);
                    const int horizontalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, tab, widget);
                    int hframe = proxy()->pixelMetric(QStyle::PM_TabBarTabHSpace, option, widget) / 2;
                    hframe = qMax(hframe, 4); // workaround KStyle returning 0 because they workaround an old bug in Qt
                    const int padding = 4;

                    QRect tr = tab->rect;

                    tr.adjust(0, 0, horizontalShift, verticalShift);

                    if (selected) {
                        tr.setBottom(tr.bottom() - verticalShift);
                        tr.setRight(tr.right() - horizontalShift);
                    }

                    const QSize size = (element == SE_TabBarTabLeftButton) ? tab->leftButtonSize : tab->rightButtonSize;
                    const int midHeight = static_cast<int>(qCeil(float(tr.height() - size.height()) / 2));

                    if (element == SE_TabBarTabLeftButton)
                        r = QRect(tab->rect.x() + hframe - padding, tab->rect.top() + midHeight, size.width(), size.height());
                    else
                        r = QRect(tab->rect.right() - size.width() - hframe + padding, tab->rect.top() + midHeight, size.width(), size.height());

                    r = visualRect(tab->direction, tab->rect, r);
                }
                return r;
            }

            return QProxyStyle::subElementRect(element, option, widget);
        }

        void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const override
        {
            if (element == CE_TabBarTabLabel) {
                if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
                    const bool selected = tab->state & State_Selected;
                    const int verticalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftVertical, tab, widget);
                    const int horizontalShift = proxy()->pixelMetric(QStyle::PM_TabBarTabShiftHorizontal, tab, widget);

                    QStyleOptionTab opt(*tab);
                    opt.shape = QTabBar::RoundedNorth;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                    const QStringList lines = opt.text.split("\n", Qt::SkipEmptyParts);
#else
                    const QStringList lines = opt.text.split("\n", QString::SkipEmptyParts);
#endif
                    const QFontMetrics fm = option->fontMetrics;

                    QList<int> lineHeights;
                    int totalHeight = 0;
                    for (const QString &line : lines) {
                        lineHeights << fm.boundingRect(line).height();
                        totalHeight += lineHeights.last();
                    }

                    QRect textRect = subElementRect(SE_TabBarTabText, &opt, widget);
                    int y = opt.rect.top() + opt.rect.height() / 2 - totalHeight / 2;

                    y += verticalShift;
                    textRect.adjust(0, 0, horizontalShift, verticalShift);

                    if (selected) {
                        textRect.setBottom(textRect.bottom() - verticalShift);
                        textRect.setRight(textRect.right() - horizontalShift);
                        y -= verticalShift;
                    }

                    for (int i = 0; i < lines.size(); ++i) {
                        painter->drawText(QRect(textRect.left(), y, textRect.width(), lineHeights[i]), Qt::AlignVCenter | Qt::AlignLeft, lines[i]);
                        y += lineHeights[i];
                    }

                    return;
                }
            }

            QProxyStyle::drawControl(element, option, painter, widget);
        }
};

OrderDetailsMdiWidget *OrderDetailsMdiWidget::getInstance()
{
    if (!OrderDetailsMdiWidget::instance)
        OrderDetailsMdiWidget::instance = new OrderDetailsMdiWidget();

    return OrderDetailsMdiWidget::instance;
}

OrderDetailsMdiWidget::OrderDetailsMdiWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::OrderDetailsMdiWidget)
{
    m_ui->setupUi(this);
    m_ui->tabWidget->tabBar()->setStyle(new HorizontalTabStyle);

    connect(m_ui->tabWidget, &QTabWidget::tabCloseRequested, [&](const int idx)
    {
        delete m_ui->tabWidget->widget(idx);

        updateWindowTitle();

        if (m_ui->tabWidget->count() == 0)
            close();
    });

    setAttribute(Qt::WA_DeleteOnClose, true);

    readSettings();
}

OrderDetailsMdiWidget::~OrderDetailsMdiWidget()
{
    OrderDetailsMdiWidget::instance = nullptr;

    delete m_ui;
    m_ui = nullptr;
}

void OrderDetailsMdiWidget::addWidget(OrderDetailsWidget *widget)
{
    const Order &order = widget->order();
    const QString tabText = QString("#%1\n%2 %3").arg(order.id).arg(order.shipping.address.firstName).arg(order.shipping.address.lastName);
    const int idx = m_ui->tabWidget->addTab(widget, tabText);
    m_ui->tabWidget->setCurrentIndex(idx);

    updateWindowTitle();
}

bool OrderDetailsMdiWidget::hasOrder(const int id) const
{
    for (int i = 0; i < m_ui->tabWidget->count(); ++i) {
        OrderDetailsWidget *orderWidget = qobject_cast<OrderDetailsWidget*>(m_ui->tabWidget->widget(i));
        if (orderWidget->order().id == id)
            return true;
    }

    return false;
}

int OrderDetailsMdiWidget::currentOrder() const
{
    OrderDetailsWidget *orderWidget = qobject_cast<OrderDetailsWidget*>(m_ui->tabWidget->currentWidget());
    if (!orderWidget)
        return -1;

    return orderWidget->order().id;
}

void OrderDetailsMdiWidget::setCurrentOrder(const int id)
{
    for (int i = 0; i < m_ui->tabWidget->count(); ++i) {
        OrderDetailsWidget *orderWidget = qobject_cast<OrderDetailsWidget*>(m_ui->tabWidget->widget(i));
        if (orderWidget->order().id != id)
            continue;

        m_ui->tabWidget->setCurrentIndex(i);
        break;
    }
}

void OrderDetailsMdiWidget::closeEvent(QCloseEvent *event)
{
    writeSettings();

    QWidget::closeEvent(event);
}

void OrderDetailsMdiWidget::updateWindowTitle()
{
    setWindowTitle(tr("Order Details Group - %n order(s)", "", m_ui->tabWidget->count()));
}

void OrderDetailsMdiWidget::readSettings()
{
    QSettings set;

    set.beginGroup("OrderDetailsMdiWidget");
    restoreGeometry(set.value("geometry").toByteArray());
    setWindowState(Qt::WindowStates(set.value("state").toInt()));
}

void OrderDetailsMdiWidget::writeSettings() const
{
    QSettings set;

    set.beginGroup("OrderDetailsMdiWidget");
    set.setValue("geometry", saveGeometry());
    set.setValue("state", (int)windowState());
}
