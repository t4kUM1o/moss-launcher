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
        setCallbackText(tr("Microsoft authorization response received. Return to Moss Launcher to complete account verification. "
                           "You can close this browser tab."));
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
