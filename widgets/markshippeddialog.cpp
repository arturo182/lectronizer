#include "markshippeddialog.h"
#include "shareddata.h"
#include "structs.h"
#include "ui_markshippeddialog.h"

#include <QPushButton>
#include <QUrl>

MarkShippedDialog::MarkShippedDialog(const Order &order, SharedData *sharedData, QWidget *parent)
    : QDialog(parent)
    , m_ui{new Ui::MarkShippedDialog}
    , m_sharedData{sharedData}
{
    m_ui->setupUi(this);

    if (sharedData->trackingUrl.isEmpty())
        m_ui->urlInfoLabel->setText(tr("Hint: You can provide a Tracking URL template in Settings so it gets automatically filled next time."));

    m_ui->orderLabel->setText(tr("Order #%1 to %2 %3").arg(order.id).arg(order.shipping.address.firstName, order.shipping.address.lastName));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    const auto updateButtons = [this]()
    {
        const bool trackingNoValid = !m_ui->trackingNoEdit->text().isEmpty();
        const bool trackingUrlValid = m_ui->trackingUrlEdit->styleSheet().isEmpty();

        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(trackingNoValid && trackingUrlValid);
    };

    connect(m_ui->trackingNoEdit, &QLineEdit::textChanged, this, [this, updateButtons](const QString &text)
    {
       if (m_sharedData->trackingUrl.isEmpty())
           return;

       QString url = m_sharedData->trackingUrl;
       url.replace("{#}", text);

       m_ui->trackingUrlEdit->setText(url);

       updateButtons();
    });

    connect(m_ui->trackingUrlEdit, &QLineEdit::textChanged, this, [this, updateButtons](const QString &text)
    {
        const bool validUrl = QUrl(text, QUrl::StrictMode).isValid();
        const bool startsWithHttp = (text.startsWith("http://", Qt::CaseInsensitive)) || (text.startsWith("https://", Qt::CaseInsensitive));

        if (!text.isEmpty() && validUrl && startsWithHttp) {
            m_ui->trackingUrlEdit->setStyleSheet("");
        } else {
            m_ui->trackingUrlEdit->setStyleSheet("color: red;");
        }

        updateButtons();
    });
}

MarkShippedDialog::~MarkShippedDialog()
{
    delete m_ui;
    m_ui = nullptr;
}

QString MarkShippedDialog::trackingNo() const
{
    return m_ui->trackingNoEdit->text();
}

QString MarkShippedDialog::trackingUrl() const
{
    return m_ui->trackingUrlEdit->text();
}
