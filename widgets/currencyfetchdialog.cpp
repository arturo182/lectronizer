#include "currencyfetchdialog.h"

#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

static const QString CurrencyUrl{"https://api.exchangerate.host/latest"};

CurrencyFetchDialog::CurrencyFetchDialog(QNetworkAccessManager *nam, QWidget *parent)
    : QProgressDialog{parent}
    , m_nam(nam)
{
    setLabelText(tr("Fetching currency rates..."));
    setCancelButtonText(QString());

    setWindowModality(Qt::WindowModal);
    setAutoClose(false);
}

CurrencyFetchDialog::~CurrencyFetchDialog()
{

}

int CurrencyFetchDialog::exec()
{
    QNetworkRequest req(CurrencyUrl);
    m_reply = m_nam->get(req);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(m_reply, &QNetworkReply::errorOccurred, m_reply, [this]()
#else
    connect(m_reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), m_reply, [this]()
#endif
    {
        setErrorMsg(m_reply->errorString());
    });

    connect(m_reply, &QNetworkReply::sslErrors, m_reply, [this](const QList<QSslError> &errors)
    {
        setErrorMsg(errors[0].errorString());
    });

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal)
    {
        setMaximum(bytesTotal);
        setValue(bytesReceived);
    });

    connect(m_reply, &QNetworkReply::finished, m_reply, [this]()
    {
        if(!m_reply)
            return;

        const QByteArray json = m_reply->readAll();

        QJsonParseError error = {};
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        if (error.error != QJsonParseError::NoError) {
            setErrorMsg(error.errorString());
            return;
        }

        const QJsonObject root = doc.object();

        if (root.value("base").toString() != "EUR") {
            setErrorMsg("Base currency is not EUR");
            return;
        }

        const QJsonObject rates = root.value("rates").toObject();
        for (auto it = rates.begin(); it != rates.end(); ++it)
            m_rates.insert(it.key(), it.value().toDouble());

        m_reply->deleteLater();
        m_reply = nullptr;

        accept();
    });

    return QProgressDialog::exec();
}

QHash<QString, qreal> CurrencyFetchDialog::rates() const
{
    return m_rates;
}

void CurrencyFetchDialog::setErrorMsg(const QString &error)
{
    setValue(maximum());
    setLabelText(tr("Failed to fetch currencies, reason:\n\"%1\"\n\nContinuing with currency conversion disabled.").arg(error));
    setCancelButtonText(tr("OK"));

    m_reply->deleteLater();
    m_reply = nullptr;
}
