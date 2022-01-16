#include "clickylineedit.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>

ClickyLineEdit::ClickyLineEdit(QWidget *parent, Qt::WindowFlags f)
    : QFrame(parent, f)
{
    setStyleSheet("QFrame { background-color: palette(base); }");
    setFrameShape(QFrame::StyledPanel);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setFrame(false);
    m_lineEdit->setReadOnly(true);

    m_pushButton = new QPushButton(this);
    m_pushButton->setEnabled(false);
    m_pushButton->setFlat(true);
    connect(m_pushButton, &QPushButton::clicked, this, [&]()
    {
        if (m_type == TypeCopy) {
            qApp->clipboard()->setText(m_lineEdit->text());
        } else if (m_type == TypeUrl) {
            const QUrl url(m_lineEdit->text());

            if (!url.isValid()) {
                QMessageBox::critical(this, tr("Error"), tr("Not a valid URL!"));
                return;
            }

            QDesktopServices::openUrl(url);
        }
    });

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pushButton);
    layout->addWidget(m_lineEdit);
}

QString ClickyLineEdit::text() const
{
    return m_lineEdit->text();
}

void ClickyLineEdit::setText(const QString &text)
{
    if (m_lineEdit->text() == text)
        return;

    m_lineEdit->setText(text);
    m_pushButton->setEnabled(!text.isEmpty());

    emit textChanged();
}

QString ClickyLineEdit::placeholderText() const
{
    return m_lineEdit->placeholderText();
}

void ClickyLineEdit::setPlaceholderText(const QString &placeholderText)
{
    if (m_lineEdit->placeholderText() == placeholderText)
        return;

    m_lineEdit->setPlaceholderText(placeholderText);

    emit placeholderTextChanged();
}

int ClickyLineEdit::type() const
{
    return m_type;
}

void ClickyLineEdit::setType(const int type)
{
    typedef typeof(m_type) Type;
    m_type = (Type)type;

    if (m_type == TypeCopy) {
        m_pushButton->setToolTip(tr("Copy"));
        m_pushButton->setIcon(QIcon(":/res/icons/page_white_copy.png"));
    } else if (m_type == TypeUrl) {
        m_pushButton->setToolTip(tr("Open URL"));
        m_pushButton->setIcon(QIcon(":/res/icons/world.png"));
    }
}
