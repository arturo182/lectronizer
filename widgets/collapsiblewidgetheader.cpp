#include "collapsiblewidgetheader.h"

#include <QBrush>
#include <QLayout>
#include <QPainter>
#include <QPen>
#include <QPen>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOption>

static const QSize ArrowSize{16, 16};

CollapsibleWidgetHeader::CollapsibleWidgetHeader(QWidget *parent)
    : QWidget{parent}
{
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
}

QWidget *CollapsibleWidgetHeader::container() const
{
    return m_container;
}

void CollapsibleWidgetHeader::setContainer(QWidget *container)
{
    // expand the old container before changing to new one
    if (m_container) {
        m_container->removeEventFilter(this);
        m_container->setMaximumHeight(m_container->sizeHint().height());
    }

    m_container = container;
    m_container->installEventFilter(this);
}

// based on QToolButton::sizeHint
QSize CollapsibleWidgetHeader::sizeHint() const
{
     ensurePolished();

     QStyleOptionToolButton opt{};
     opt.text = m_title;
     opt.iconSize = ArrowSize;
     opt.toolButtonStyle = Qt::ToolButtonTextBesideIcon;

     QFontMetrics fm = fontMetrics();
     int w = opt.iconSize.width();
     int h = opt.iconSize.height();

     QSize textSize = fm.size(Qt::TextShowMnemonic, opt.text);
     textSize.setWidth(textSize.width() + fm.horizontalAdvance(QLatin1Char(' ')) * 2);

     w += 4 + textSize.width();
     if (textSize.height() > h)
         h = textSize.height();

     opt.rect.setSize(QSize(w, h));

     return style()->sizeFromContents(QStyle::CT_ToolButton, &opt, QSize(w, h), this);
}

bool CollapsibleWidgetHeader::isExpanded() const
{
    return m_expanded;
}

void CollapsibleWidgetHeader::setExpanded(const bool expanded)
{
    m_expanded = expanded;
    update();

    if (!m_container)
        return;

    if (m_expanded) {
        m_container->setMaximumHeight(m_container->sizeHint().height());
    } else {
        m_container->setMaximumHeight(0);
    }
}

void CollapsibleWidgetHeader::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter p(this);

    p.setPen(QPen(palette().color(QPalette::Dark)));
    p.drawLine(rect().topLeft(), rect().topRight());

    QStyleOption arrowOpt{};
    arrowOpt.state = QStyle::State_Enabled;
    arrowOpt.rect = { 0, 0, ArrowSize.width() + 4, height() }; // 4 is currently hardcoded in QToolButton::sizeHint()
    style()->drawPrimitive(m_expanded ? QStyle::PE_IndicatorArrowDown : QStyle::PE_IndicatorArrowRight, &arrowOpt, &p, this);

    const int alignment = Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic;
    const QRect textRect = rect().adjusted(arrowOpt.rect.width(), 0, 0, 0);
    style()->drawItemText(&p, QStyle::visualRect(Qt::LeftToRight, rect(), textRect), alignment, palette(), true, m_title, QPalette::ButtonText);
}

void CollapsibleWidgetHeader::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        setExpanded(!m_expanded);
}

bool CollapsibleWidgetHeader::eventFilter(QObject *obj, QEvent *ev)
{
    // recalculate height when layout requests it
    if ((obj == m_container) && (ev->type() == QEvent::LayoutRequest))
        setExpanded(isExpanded());

    return QWidget::eventFilter(obj, ev);
}

QString CollapsibleWidgetHeader::title() const
{
    return m_title;
}

void CollapsibleWidgetHeader::setTitle(const QString &title)
{
    m_title = title;

    update();
}
