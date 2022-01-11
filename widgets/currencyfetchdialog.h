#pragma once

#include <QHash>
#include <QProgressDialog>
#include <QWidget>

class QNetworkAccessManager;
class QNetworkReply;

class CurrencyFetchDialog : public QProgressDialog
{
    Q_OBJECT

    public:
        explicit CurrencyFetchDialog(QNetworkAccessManager *nam, QWidget *parent = nullptr);
        ~CurrencyFetchDialog() override;

        int exec() override;

        QHash<QString, qreal> rates() const;

    private:
        void setErrorMsg(const QString &error);

    private:
        QNetworkAccessManager *m_nam{};
        QHash<QString, qreal> m_rates{};
        QNetworkReply *m_reply{};
};

