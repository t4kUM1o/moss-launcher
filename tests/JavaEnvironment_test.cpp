// SPDX-License-Identifier: GPL-3.0-only
// Moss fork regression tests, added 2026-10-02.
#include <QDir>
#include <QTest>
#include "BuildConfig.h"
#include "java/JavaUtils.h"

QStringList addJavasFromEnv(QList<QString> javas);

class JavaEnvironmentTest : public QObject {
    Q_OBJECT
    QByteArray savedHome;
    QByteArray savedPath;
    QByteArray savedCustom;
    bool hadHome;
    bool hadPath;
    bool hadCustom;
    QByteArray customName;

   private slots:
    void init()
    {
        customName = (BuildConfig.LAUNCHER_ENVNAME + "_JAVA_PATHS").toUtf8();
        hadHome = qEnvironmentVariableIsSet("JAVA_HOME");
        hadPath = qEnvironmentVariableIsSet("PATH");
        hadCustom = qEnvironmentVariableIsSet(customName.constData());
        savedHome = qgetenv("JAVA_HOME");
        savedPath = qgetenv("PATH");
        savedCustom = qgetenv(customName.constData());
        qunsetenv("JAVA_HOME");
        qunsetenv("PATH");
        qunsetenv(customName.constData());
    }
    void cleanup()
    {
        if (hadHome) qputenv("JAVA_HOME", savedHome); else qunsetenv("JAVA_HOME");
        if (hadPath) qputenv("PATH", savedPath); else qunsetenv("PATH");
        if (hadCustom) qputenv(customName.constData(), savedCustom); else qunsetenv(customName.constData());
    }
    void javaHomeWithQuotesAndSpaces()
    {
        qputenv("JAVA_HOME", "\"C:/Java Home 26\"");
        auto expected = QDir("C:/Java Home 26").absoluteFilePath("bin/" + JavaUtils::javaExecutable);
        QVERIFY(addJavasFromEnv({}).contains(expected));
    }
    void emptyEnvironmentDoesNotSearchCurrentDirectory()
    {
        QVERIFY(addJavasFromEnv({}).isEmpty());
    }
    void explicitPathsArePreserved()
    {
        qputenv(customName.constData(), "C:/custom-java/bin/javaw.exe");
        QVERIFY(addJavasFromEnv({ "existing-java" }).contains("existing-java"));
        QVERIFY(addJavasFromEnv({}).contains("C:/custom-java/bin/javaw.exe"));
    }
    void allWindowsPathEntriesAreIncluded()
    {
#ifdef Q_OS_WIN
        qputenv("PATH", ";\"C:/Java 26/bin\";C:/Java21/bin;");
        auto paths = addJavasFromEnv({});
        QVERIFY(paths.contains("C:/Java 26/bin/javaw.exe"));
        QVERIFY(paths.contains("C:/Java21/bin/javaw.exe"));
        QVERIFY(!paths.contains("/javaw.exe"));
#endif
    }
};

QTEST_GUILESS_MAIN(JavaEnvironmentTest)
#include "JavaEnvironment_test.moc"
