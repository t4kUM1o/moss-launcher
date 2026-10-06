// SPDX-License-Identifier: GPL-3.0-only
// Offline only: no browser, remote authentication, account or elevation required.
#include <QSignalSpy>
#include <QTest>
#include "minecraft/auth/MossAuthMessages.h"
#include "minecraft/auth/MossLoginAttempt.h"

class FakeLoginTask : public Task {
   public:
    FakeLoginTask() : Task(false) { setAbortable(true); }
    int starts = 0;
    int aborts = 0;
    void fail() { emitFailed("fake failure"); }
    void succeed() { emitSucceeded(); }
    bool abort() override { ++aborts; return Task::abort(); }
   protected:
    void executeTask() override { ++starts; }
};

class MossLoginFlowTest : public QObject {
    Q_OBJECT
   private slots:
    void startsOnlyOneSelectedMethod()
    {
        MossLoginAttempt attempt;
        auto browser = makeShared<FakeLoginTask>();
        auto code = makeShared<FakeLoginTask>();
        attempt.replace(browser);
        QTRY_COMPARE(browser->starts, 1);
        QCOMPARE(code->starts, 0);
        attempt.replace(code);
        QCOMPARE(browser->aborts, 1);
        QVERIFY(!browser->isRunning());
        QTRY_COMPARE(code->starts, 1);
        attempt.cancel();
    }

    void switchingBeforeQueuedStartDoesNotStartOldMethod()
    {
        MossLoginAttempt attempt;
        auto browser = makeShared<FakeLoginTask>();
        auto code = makeShared<FakeLoginTask>();
        const auto old = attempt.replace(browser);
        const auto current = attempt.replace(code);
        QVERIFY(!attempt.isCurrent(old));
        QVERIFY(attempt.isCurrent(current));
        QTRY_COMPARE(code->starts, 1);
        QCOMPARE(browser->starts, 0);
        attempt.cancel();
    }

    void queuedOldFailureCannotOverwriteNewAttempt()
    {
        MossLoginAttempt attempt;
        auto browser = makeShared<FakeLoginTask>();
        auto code = makeShared<FakeLoginTask>();
        int failureUpdates = 0;
        const auto old = attempt.replace(browser);
        connect(browser.get(), &Task::failed, &attempt, [&] {
            if (attempt.isCurrent(old))
                ++failureUpdates;
        }, Qt::QueuedConnection);
        QTRY_COMPARE(browser->starts, 1);
        browser->fail(); // Queue a delivery while the old attempt is still current.
        attempt.replace(code);
        QTRY_COMPARE(code->starts, 1);
        QCOMPARE(failureUpdates, 0);
        attempt.cancel();
    }

    void cancelInvalidatesBeforeAbortedSignal()
    {
        MossLoginAttempt attempt;
        auto browser = makeShared<FakeLoginTask>();
        const auto generation = attempt.replace(browser);
        bool validDuringAbort = true;
        connect(browser.get(), &Task::aborted, this, [&] { validDuringAbort = attempt.isCurrent(generation); });
        QTRY_COMPARE(browser->starts, 1);
        attempt.cancel();
        QVERIFY(!validDuringAbort);
        QCOMPARE(browser->aborts, 1);
        QVERIFY(!attempt.task());
        attempt.cancel(); // Safe to close/destruct more than once.
        QCOMPARE(browser->aborts, 1);
    }

    void closingBeforeStartMakesNoRequest()
    {
        auto browser = makeShared<FakeLoginTask>();
        {
            MossLoginAttempt attempt;
            attempt.replace(browser);
        }
        QCoreApplication::processEvents();
        QCOMPARE(browser->starts, 0);
    }

    void successCanBeObservedWithoutSecondaryFailure()
    {
        MossLoginAttempt attempt;
        auto browser = makeShared<FakeLoginTask>();
        const auto generation = attempt.replace(browser);
        int successes = 0;
        connect(browser.get(), &Task::succeeded, &attempt, [&] {
            if (attempt.isCurrent(generation))
                ++successes;
        });
        QTRY_COMPARE(browser->starts, 1);
        browser->succeed();
        QCOMPARE(successes, 1);
        attempt.cancel();
        QCOMPARE(browser->aborts, 0);
    }

    void retryUsesFreshAttemptAndCannotReuseOldGeneration()
    {
        MossLoginAttempt attempt;
        auto first = makeShared<FakeLoginTask>();
        const auto old = attempt.replace(first);
        QTRY_COMPARE(first->starts, 1);
        first->fail();
        auto retry = makeShared<FakeLoginTask>();
        const auto current = attempt.replace(retry);
        QVERIFY(!attempt.isCurrent(old));
        QVERIFY(attempt.isCurrent(current));
        QTRY_COMPARE(retry->starts, 1);
        QCOMPARE(first->starts, 1);
        attempt.cancel();
    }

    void failureMessagesDoNotRepeatRawServerSecrets()
    {
        const auto device = MossAuthMessages::deviceFailure(
            R"({"error":"invalid_client","error_description":"PRIVATE_TOKEN <script>"})",
            QNetworkReply::AuthenticationRequiredError);
        QVERIFY(device.contains("パブリック"));
        QVERIFY(!device.contains("PRIVATE_TOKEN"));
        QVERIFY(!device.contains("<script>"));
        QVERIFY(!device.contains("期限"));
        const auto minecraft = MossAuthMessages::minecraftFailure(QNetworkReply::ContentAccessDenied);
        QVERIFY(minecraft.contains("Minecraft API"));
        QVERIFY(minecraft.contains("断定できません"));
        QVERIFY(!minecraft.contains("期限"));
    }
};

QTEST_GUILESS_MAIN(MossLoginFlowTest)
#include "MossLoginFlow_test.moc"
