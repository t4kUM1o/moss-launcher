// SPDX-License-Identifier: GPL-3.0-only
// Moss modification, 2026-10-03: safe, actionable failure messages.
#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QObject>

namespace MossAuthMessages {
inline QString deviceFailure(const QByteArray& response, QNetworkReply::NetworkError error)
{
    const auto code = QJsonDocument::fromJson(response).object().value("error").toString();
    if (code == "invalid_client" || code == "unauthorized_client" || error == QNetworkReply::AuthenticationRequiredError) {
        return QObject::tr("Microsoftのコード入力ログインが拒否されました。Azureのアプリ登録で、クライアントIDと"
                           "「パブリック クライアント フローを許可する」を確認してください。"
                           "ブラウザ方式に切り替えて試すこともできます。");
    }
    return QObject::tr("Microsoftのコード入力ログインを開始できませんでした。ネット接続やアプリ登録の設定を確認し、"
                       "再試行するかブラウザ方式に切り替えてください。");
}

inline QString minecraftFailure(QNetworkReply::NetworkError error)
{
    if (error == QNetworkReply::ContentAccessDenied || error == QNetworkReply::AuthenticationRequiredError) {
        return QObject::tr("Microsoft・Xboxの認証後、Minecraftサービスへのアクセスが拒否されました。"
                           "Moss用アプリのMinecraft API利用承認と認証設定を確認してください。"
                           "このエラーだけでは未承認と断定できません。ログイン方式を変えるだけでは解決しない場合があります。");
    }
    return QObject::tr("Minecraftの認証サービスに接続できませんでした。ネット接続やサービスの状態を確認して再試行してください。");
}
} // namespace MossAuthMessages
