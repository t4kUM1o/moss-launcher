// SPDX-License-Identifier: GPL-3.0-only
// Moss fork modification, 2026-10-03: one local redirect for portable/installed builds.
#pragma once

#include <QHostAddress>
#include <QOAuthHttpServerReplyHandler>
#include <QUrl>

class MossOAuthReplyHandler final : public QOAuthHttpServerReplyHandler {
   public:
    explicit MossOAuthReplyHandler(QObject* parent = nullptr)
        : QOAuthHttpServerReplyHandler(QHostAddress::LocalHost, 0, parent)
    {
        setCallbackPath("/oauth/microsoft");
        // A received Microsoft response is not proof of Minecraft ownership.
        setCallbackText(tr("Microsoftからの応答を受け取りました。Moss Launcherへ戻って認証結果を確認してください。"
                           "Minecraftの認証やゲームの所有確認は、この時点では完了していません。"
                           "このブラウザタブは閉じてかまいません。"));
    }

    QString callback() const override
    {
        // Entra accepts localhost in its desktop-platform UI and ignores its
        // dynamic port when matching http://localhost/oauth/microsoft.
        // Listening remains explicitly restricted to the IPv4 loopback address.
        QUrl url(QOAuthHttpServerReplyHandler::callback());
        url.setHost("localhost");
        return url.toString(QUrl::FullyEncoded);
    }
};
