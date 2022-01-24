#pragma once

#include <QWidget>
#include <QDebug>
#include <QPaintEvent>

class CollapsibleWidgetHeader : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)

    public:
        explicit CollapsibleWidgetHeader(QWidget *parent = nullptr);

        QWidget *container() const;
        void setContainer(QWidget *container);

        QSize sizeHint() const override;

        bool isExpanded() const;
        void setExpanded(const bool expanded);

        QString title() const;
        void setTitle(const QString &title);

    protected:
        void paintEvent(QPaintEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        bool eventFilter(QObject *obj, QEvent *ev) override;

    private:
        QWidget *m_container{};
        bool m_expanded{true};
        QString m_title{};
};
