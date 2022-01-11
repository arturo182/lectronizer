#pragma once

#include <QFrame>
#include <QIcon>

class QLineEdit;
class QPushButton;

class ClickyLineEdit : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int type READ type WRITE setType)

    public:
        explicit ClickyLineEdit(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

        QString text() const;
        void setText(const QString &text);

        QString placeholderText() const;
        void setPlaceholderText(const QString &placeholderText);

        int type() const;
        void setType(const int type);

    signals:
        void textChanged();
        void placeholderTextChanged();

    private:
        enum { TypeCopy, TypeUrl } m_type{TypeCopy};
        QLineEdit *m_lineEdit{};
        QPushButton *m_pushButton{};
};

