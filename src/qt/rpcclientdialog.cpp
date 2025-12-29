// Copyright (c) 2024 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcclientdialog.h"

#include "chainparamsbase.h"
#include "rpc/protocol.h"
#include "util.h"
#include "utilstrencodings.h"
#include "netbase.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr const char *kDefaultRpcConnect = "127.0.0.1";

QString defaultRpcHost()
{
    return QString::fromStdString(gArgs.GetArg("-rpcconnect", kDefaultRpcConnect));
}

QString defaultRpcPort()
{
    int port = BaseParams().RPCPort();
    std::string host;
    SplitHostPort(gArgs.GetArg("-rpcconnect", kDefaultRpcConnect), port, host);
    port = gArgs.GetArg("-rpcport", port);
    return QString::number(port);
}

QString defaultWalletPath()
{
    std::string walletName = gArgs.GetArg("-rpcwallet", "");
    if (walletName.empty()) {
        return QString();
    }
    return QString::fromStdString("/wallet/" + walletName);
}
}

RPCClientDialog::RPCClientDialog(QWidget *parent)
    : QWidget(parent)
    , hostEdit(new QLineEdit(defaultRpcHost(), this))
    , portEdit(new QLineEdit(defaultRpcPort(), this))
    , userEdit(new QLineEdit(QString::fromStdString(gArgs.GetArg("-rpcuser", "")), this))
    , passwordEdit(new QLineEdit(QString::fromStdString(gArgs.GetArg("-rpcpassword", "")), this))
    , walletEdit(new QLineEdit(defaultWalletPath(), this))
    , methodEdit(new QLineEdit(this))
    , paramsEdit(new QTextEdit(this))
    , responseEdit(new QTextEdit(this))
    , sendButton(new QPushButton(tr("Send"), this))
    , network(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Raven RPC Client"));
    resize(900, 600);

    passwordEdit->setEchoMode(QLineEdit::Password);
    paramsEdit->setPlaceholderText(tr("JSON array or object, e.g. [] or [\"param\"]"));
    paramsEdit->setPlainText("[]");
    responseEdit->setReadOnly(true);

    auto *formLayout = new QFormLayout();
    formLayout->addRow(tr("Host"), hostEdit);
    formLayout->addRow(tr("Port"), portEdit);
    formLayout->addRow(tr("RPC User"), userEdit);
    formLayout->addRow(tr("RPC Password"), passwordEdit);
    formLayout->addRow(tr("Wallet path (optional)"), walletEdit);
    formLayout->addRow(tr("Method"), methodEdit);
    formLayout->addRow(tr("Params"), paramsEdit);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(sendButton);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addLayout(buttonLayout);
    layout->addWidget(new QLabel(tr("Response"), this));
    layout->addWidget(responseEdit);

    connect(sendButton, &QPushButton::clicked, this, &RPCClientDialog::sendRequest);
    connect(network, &QNetworkAccessManager::finished, this, &RPCClientDialog::requestFinished);
}

QString RPCClientDialog::buildEndpoint() const
{
    QString walletPath = walletEdit->text().trimmed();
    if (!walletPath.isEmpty()) {
        if (!walletPath.startsWith('/')) {
            walletPath.prepend('/');
        }
        return walletPath;
    }
    return QStringLiteral("/");
}

void RPCClientDialog::sendRequest()
{
    const QString method = methodEdit->text().trimmed();
    if (method.isEmpty()) {
        appendOutput(tr("Error: method is required."));
        return;
    }

    QJsonValue paramsValue = QJsonArray();
    const QString rawParams = paramsEdit->toPlainText().trimmed();
    if (!rawParams.isEmpty()) {
        QJsonParseError parseError;
        const QJsonDocument paramsDoc = QJsonDocument::fromJson(rawParams.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            appendOutput(tr("Error parsing params: %1").arg(parseError.errorString()));
            return;
        }
        if (!paramsDoc.isArray() && !paramsDoc.isObject()) {
            appendOutput(tr("Params must be a JSON array or object."));
            return;
        }
        paramsValue = paramsDoc.isArray() ? QJsonValue(paramsDoc.array()) : QJsonValue(paramsDoc.object());
    }

    QJsonObject requestObject;
    requestObject.insert("jsonrpc", "1.0");
    requestObject.insert("id", "raven-qt");
    requestObject.insert("method", method);
    requestObject.insert("params", paramsValue);

    QUrl url;
    url.setScheme("http");
    url.setHost(hostEdit->text().trimmed());
    url.setPort(portEdit->text().trimmed().toInt());
    url.setPath(buildEndpoint());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    std::string auth;
    if (passwordEdit->text().isEmpty()) {
        if (!GetAuthCookie(&auth)) {
            appendOutput(tr("Error: missing RPC credentials (set rpcuser/rpcpassword or ensure cookie exists)."));
            return;
        }
    } else {
        auth = userEdit->text().toStdString() + ":" + passwordEdit->text().toStdString();
    }

    const QByteArray authHeader = QByteArray::fromStdString("Basic " + EncodeBase64(auth));
    request.setRawHeader("Authorization", authHeader);

    sendButton->setEnabled(false);
    const QJsonDocument requestDoc(requestObject);
    network->post(request, requestDoc.toJson(QJsonDocument::Compact));
}

void RPCClientDialog::requestFinished(QNetworkReply *reply)
{
    sendButton->setEnabled(true);

    QByteArray payload = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError) {
        appendOutput(tr("HTTP error %1: %2").arg(statusCode).arg(reply->errorString()));
    }

    if (payload.isEmpty()) {
        appendOutput(tr("Empty response."));
        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error == QJsonParseError::NoError) {
        appendOutput(QString::fromUtf8(responseDoc.toJson(QJsonDocument::Indented)));
    } else {
        appendOutput(QString::fromUtf8(payload));
    }

    reply->deleteLater();
}

void RPCClientDialog::appendOutput(const QString &text)
{
    responseEdit->append(text);
}
