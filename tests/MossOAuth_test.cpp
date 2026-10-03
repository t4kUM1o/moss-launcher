// SPDX-License-Identifier: GPL-3.0-only
// Offline Moss auth tests. Never open a browser, sign in, or call Microsoft.
#include <QOAuth2AuthorizationCodeFlow>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTest>
#include <QUrlQuery>
#include "BuildConfig.h"
#include "minecraft/auth/MossOAuthReplyHandler.h"

class MossOAuthTest : public QObject {
    Q_OBJECT
   private slots:
    void redirectIsLocalAndHasStablePath()
    {
        MossOAuthReplyHandler handler;
        QVERIFY(handler.isListening());
        const QUrl callback(handler.callback());
        QCOMPARE(callback.scheme(), QString("http"));
        QCOMPARE(callback.host(), QString("localhost"));
        QCOMPARE(callback.path(), QString("/oauth/microsoft"));
        QVERIFY(callback.port() > 0);
        auto server = handler.findChild<QTcpServer*>();
        QVERIFY(server);
        QCOMPARE(server->serverAddress(), QHostAddress(QHostAddress::LocalHost));
        QVERIFY(!handler.callbackText().contains("<script"));
        QVERIFY(!handler.callbackText().contains("prismlauncher.org"));
        handler.close();
        QVERIFY(!handler.isListening());
    }

    void authorizationUsesPublicClientStateAndPkce()
    {
        QVERIFY(!BuildConfig.MSA_CLIENT_ID.isEmpty());
        QCOMPARE(BuildConfig.MSA_CLIENT_ID.trimmed(), BuildConfig.MSA_CLIENT_ID);
        MossOAuthReplyHandler handler;
        QOAuth2AuthorizationCodeFlow flow;
        flow.setReplyHandler(&handler);
        flow.setAuthorizationUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"));
        flow.setAccessTokenUrl(QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"));
        flow.setClientIdentifier(BuildConfig.MSA_CLIENT_ID);
        flow.setScope("XboxLive.SignIn XboxLive.offline_access");
        flow.setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256);
        QSignalSpy browser(&flow, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser);
        flow.grant(); // Only builds the authorization URL: no browser or remote requests.
        QCOMPARE(browser.count(), 1);
        QUrl url = browser.at(0).at(0).toUrl();
        QCOMPARE(url.host(), QString("login.microsoftonline.com"));
        QCOMPARE(url.path(), QString("/consumers/oauth2/v2.0/authorize"));
        QUrlQuery query(url);
        QVERIFY(query.hasQueryItem("scope"));
        QCOMPARE(query.queryItemValue("client_id"), BuildConfig.MSA_CLIENT_ID);
        QCOMPARE(query.queryItemValue("redirect_uri", QUrl::FullyDecoded), handler.callback());
        QCOMPARE(query.queryItemValue("response_type"), QString("code"));
        QCOMPARE(query.queryItemValue("code_challenge_method"), QString("S256"));
        QVERIFY(!query.queryItemValue("state").isEmpty());
        QVERIFY(!query.queryItemValue("code_challenge").isEmpty());
        QVERIFY(!query.hasQueryItem("client_secret"));
        QVERIFY(!query.hasQueryItem("code_verifier"));
    }

    void mismatchedStateRejectsTheCallback()
    {
        MossOAuthReplyHandler handler;
        QOAuth2AuthorizationCodeFlow flow;
        flow.setReplyHandler(&handler);
        // No external service is contacted even if this assertion regresses.
        flow.setAuthorizationUrl(QUrl("http://127.0.0.1:1/authorize"));
        flow.setAccessTokenUrl(QUrl("http://127.0.0.1:1/token"));
        flow.setClientIdentifier("offline-test-client");
        QSignalSpy failed(&flow, &QOAuth2AuthorizationCodeFlow::requestFailed);
        QSignalSpy granted(&flow, &QOAuth2AuthorizationCodeFlow::granted);
        flow.grant();
        handler.callbackReceived({ { "code", "offline-test-code" }, { "state", "incorrect-state" } });
        QCOMPARE(failed.count(), 1);
        QCOMPARE(granted.count(), 0);
        QVERIFY(flow.token().isEmpty());
    }
};

QTEST_GUILESS_MAIN(MossOAuthTest)
#include "MossOAuth_test.moc"
