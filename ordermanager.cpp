#include "ordermanager.h"
#include "shareddata.h"
#include "sqlmanager.h"

#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

static const int FetchSize = 50;

QString OrderManager::ApiUrl{"https://lectronz.com/api/v1/orders"};

OrderManager::OrderManager(QNetworkAccessManager *nam, SharedData *shared, SqlManager *sqlMgr, QWidget *parent)
    : QObject(qobject_cast<QObject*>(parent))
    , m_nam{nam}
    , m_shared{shared}
    , m_sqlMgr{sqlMgr}
{
    m_progressDlg = new QProgressDialog(parent);
    m_progressDlg->setWindowModality(Qt::WindowModal);
    m_progressDlg->setAutoClose(false);
    m_progressDlg->setAutoReset(false);
    m_progressDlg->setMinimumDuration(0);
    resetProgressDlg();

    connect(this, &OrderManager::orderUpdated, this, [this](const Order &order)
    {
        m_sqlMgr->save(order);
    });
}

OrderManager::~OrderManager()
{
    delete m_progressDlg;
    m_progressDlg = nullptr;
}

void OrderManager::refresh(const bool hidden)
{
    resetProgressDlg();

    m_progressDlg->setAutoClose(hidden);
    m_progressDlg->setHidden(hidden);

    m_newOrders = 0;
    m_updatedOrders = 0;

    // we fetch the first batch, processFetch() will do the following fetches if needed
    fetch(0, FetchSize);
}

bool OrderManager::contains(const int id) const
{
    return m_orders.contains(id);
}

Order &OrderManager::order(const int id)
{
    return m_orders[id];
}

QList<int> OrderManager::orderIds() const
{
    return m_orders.keys();
}

void OrderManager::markShipped(const int id, const QString &trackingNo, const QString &trackingUrl)
{
    if (!contains(id)) {
        QMessageBox::warning(nullptr, tr("Bad order"), tr("Order doesn't exist."));
        return;
    }

    if (m_reply) {
        QMessageBox::warning(nullptr, tr("Bad state"), tr("Can't do while refreshing orders. Please retry after it's done."));
        return;
    }

    resetProgressDlg();
    m_progressDlg->setAutoClose(false);
    m_progressDlg->setHidden(false);

    QUrl url(ApiUrl + "/" + QString::number(id));

    QNetworkRequest req(url);
    req.setRawHeader("Content-Type", "application/json");
    req.setRawHeader("Accept", "*/*");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(m_shared->apiKey).toUtf8());

    const QJsonObject rootObj({
        { "status",        "fulfilled" },
        { "tracking_code", trackingNo  },
        { "tracking_url",  trackingUrl },
    });

    m_reply = m_nam->put(req, QJsonDocument(rootObj).toJson(QJsonDocument::Compact));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(m_reply, &QNetworkReply::errorOccurred, m_reply, [this]()
#else
    connect(m_reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), m_reply, [this]()
#endif
    {
        if (m_reply->error() == QNetworkReply::AuthenticationRequiredError) {
            setErrorMsg(tr("Authorization error, maybe double-check your API key!"));
        } else {
            setErrorMsg(m_reply->errorString());
        }
    });

    connect(m_reply, &QNetworkReply::sslErrors, m_reply, [this](const QList<QSslError> &errors)
    {
        setErrorMsg(errors[0].errorString());
    });

    connect(m_reply, &QNetworkReply::finished, m_reply, [this]()
    {
        if(!m_reply)
            return;

        const QByteArray json = m_reply->readAll();
        qDebug() << "finished" << json;

        QJsonParseError error = {};
        QJsonDocument doc = QJsonDocument::fromJson(json, &error);
        if (error.error != QJsonParseError::NoError) {
            setErrorMsg(error.errorString());
            return;
        }

        const QJsonObject root = doc.object();

        // update the order
        Order order = parseJsonOrder(root);
        m_sqlMgr->restore(order);
        m_orders.insert(order.id, order);
        emit orderUpdated(order);

        // cleanup reply
        m_reply->deleteLater();
        m_reply = nullptr;

        // close the dialog
        m_progressDlg->setValue(m_progressDlg->maximum());
        if (m_progressDlg->isVisible())
            m_progressDlg->hide();
    });
}

void OrderManager::resetProgressDlg()
{
    m_progressDlg->setValue(0);
    m_progressDlg->setMaximum(0);
    m_progressDlg->setLabelText(tr("Refreshing orders..."));
    m_progressDlg->setCancelButtonText(QString());
}

void OrderManager::fetch(const int offset, const int limit)
{
    m_progressDlg->setMaximum(m_progressDlg->maximum() + limit);

    if (m_reply) {
        qDebug() << "Trying to fetch while previous request still active!";
        return;
    }

    if (m_shared->apiKey.isEmpty()) {
        setErrorMsg(tr("You need to fill in your API key first. Go to Tools->Settings..., paste it there and try again."));
        return;
    }

    QUrlQuery query;
    query.addQueryItem("offset", QString::number(offset));
    query.addQueryItem("limit", QString::number(limit));

    QUrl url(ApiUrl);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("Accept", "*/*");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(m_shared->apiKey).toUtf8());

    m_reply = m_nam->get(req);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(m_reply, &QNetworkReply::errorOccurred, m_reply, [this]()
#else
    connect(m_reply, qOverload<QNetworkReply::NetworkError>(&QNetworkReply::error), m_reply, [this]()
#endif
    {
        if (m_reply->error() == QNetworkReply::AuthenticationRequiredError) {
            setErrorMsg(tr("Authorization error, maybe double-check your API key!"));
        } else {
            setErrorMsg(m_reply->errorString());
        }
    });

    connect(m_reply, &QNetworkReply::sslErrors, m_reply, [this](const QList<QSslError> &errors)
    {
        setErrorMsg(errors[0].errorString());
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

        m_reply->deleteLater();
        m_reply = nullptr;

        processFetch(root);
    });
}

void OrderManager::processFetch(const QJsonObject &root)
{
    const int offset = root.value("offset").toInt();
    const int totalOrders  = root.value("total_count").toInt();

    const QJsonArray jsonOrders = root.value("orders").toArray();
    const int count  = jsonOrders.size();
    const int lastOrderNum = offset + count;

    for (const QJsonValue &val : jsonOrders) {
        Order order = parseJsonOrder(val);
        m_sqlMgr->restore(order);

        if (!contains(order.id)) {
            m_orders.insert(order.id, order);
            emit orderReceived(order);

            m_newOrders += 1;
        } else if (order != m_orders[order.id]) {
            m_orders.insert(order.id, order);
            emit orderUpdated(order);

            m_updatedOrders += 1;
        }
    }

    if (lastOrderNum < totalOrders) {
        const int size = std::min(FetchSize, totalOrders - lastOrderNum);

        fetch(lastOrderNum, size);

        // fetch() updates maximum(), we set value after that
        m_progressDlg->setValue(m_progressDlg->value() + count);
    } else {
        // we're done
        m_progressDlg->setValue(m_progressDlg->maximum());
        if (m_progressDlg->isVisible())
            m_progressDlg->hide();

        emit refreshCompleted(m_newOrders, m_updatedOrders);
    }
}

void OrderManager::setErrorMsg(const QString &error)
{
    m_progressDlg->setValue(m_progressDlg->maximum());
    m_progressDlg->setLabelText(tr("Failed to finish request, reason:\n\"%1\"\n\nTry again, maybe wait some time, or check if the website is up,\notherwise complain on Discord I guess.").arg(error));
    m_progressDlg->setCancelButtonText(tr("OK"));
    m_progressDlg->show();

    if (!m_reply) {
        qDebug() << Q_FUNC_INFO << "called with m_reply == nullptr, highly sus";
        return;
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    emit refreshFailed(error);
}
