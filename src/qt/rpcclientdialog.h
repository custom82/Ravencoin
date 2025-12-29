// Copyright (c) 2024 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_QT_RPCCLIENTDIALOG_H
#define RAVEN_QT_RPCCLIENTDIALOG_H

#include <QWidget>

class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QTextEdit;

class RPCClientDialog : public QWidget
{
    Q_OBJECT

public:
    explicit RPCClientDialog(QWidget *parent = nullptr);

private Q_SLOTS:
    void sendRequest();
    void requestFinished(QNetworkReply *reply);

private:
    void appendOutput(const QString &text);
    QString buildEndpoint() const;

    QLineEdit *hostEdit;
    QLineEdit *portEdit;
    QLineEdit *userEdit;
    QLineEdit *passwordEdit;
    QLineEdit *walletEdit;
    QLineEdit *methodEdit;
    QTextEdit *paramsEdit;
    QTextEdit *responseEdit;
    QPushButton *sendButton;
    QNetworkAccessManager *network;
};

#endif // RAVEN_QT_RPCCLIENTDIALOG_H
