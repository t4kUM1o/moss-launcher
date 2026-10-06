// SPDX-License-Identifier: GPL-3.0-only
// Offline widget checks; no network requests or real instance changes.
#include <QTest>
#include <QFontDatabase>
#include "ui/widgets/MossLoaderHelp.h"

class MossLoaderHelpTest : public QObject {
    Q_OBJECT
   private slots:
    void initTestCase()
    {
#ifdef Q_OS_WIN
        // The offscreen platform has no system font fallback. Load a font only
        // into this test process so the optional Japanese preview is readable.
        if (!qEnvironmentVariable("MOSSPRISM_LOADER_HELP_PREVIEW").isEmpty()) {
            const auto fontId = QFontDatabase::addApplicationFont("C:/Windows/Fonts/meiryo.ttc");
            QVERIFY(fontId >= 0);
            QApplication::setFont(QFont(QFontDatabase::applicationFontFamilies(fontId).first(), 9));
        }
#endif
    }

    void targetAndVersionMeaning()
    {
        MossLoaderHelp help;
        help.setContext("26.3", "Forge");
        auto* target = help.findChild<QLabel*>("mossLoaderTarget");
        auto* versions = help.findChild<QLabel*>("mossLoaderVersions");
        auto* mods = help.findChild<QLabel*>("mossLoaderModCompatibility");
        QVERIFY(target && versions && mods);
        QVERIFY(target->text().contains("26.3"));
        QVERIFY(target->text().contains("Forge"));
        QVERIFY(versions->text().contains(QStringLiteral("Forge自身のバージョン")));
        QVERIFY(mods->text().contains(QStringLiteral("Minecraft 26.3・Forge対応")));
        QVERIFY(mods->text().contains(QStringLiteral("互換性は保証されません")));
        QVERIFY(help.toolTip().contains(QStringLiteral("同じ候補")));
        QVERIFY(target->font().bold());
        for (auto* label : { target, versions, mods }) {
            QCOMPARE(label->textFormat(), Qt::PlainText);
            QVERIFY(label->wordWrap());
        }
    }

    void contextSwitchAndPlainText()
    {
        MossLoaderHelp help;
        help.setContext("26.3", "Forge");
        help.setContext("1.21.1", "Fabric");
        auto* target = help.findChild<QLabel*>("mossLoaderTarget");
        auto* mods = help.findChild<QLabel*>("mossLoaderModCompatibility");
        QVERIFY(target->text().contains("1.21.1"));
        QVERIFY(!target->text().contains("26.3"));
        QVERIFY(mods->text().contains("Fabric"));
        QVERIFY(!mods->text().contains("Forge"));
        help.setContext("<b>26.3</b>", "Forge");
        QCOMPARE(target->textFormat(), Qt::PlainText);
        QVERIFY(target->text().contains("<b>26.3</b>"));
        help.setContext({}, "Quilt");
        QVERIFY(target->text().contains(QStringLiteral("未設定")));
    }

    void wrappedLayout()
    {
        MossLoaderHelp help;
        help.setContext("26.3", "Forge");
        help.resize(596, help.sizeHint().height());
        help.show();
        QVERIFY(QTest::qWaitForWindowExposed(&help));
        help.layout()->activate();
        for (auto* label : help.findChildren<QLabel*>()) {
            QVERIFY(label->height() >= label->heightForWidth(label->width()));
        }
        const auto preview = qEnvironmentVariable("MOSSPRISM_LOADER_HELP_PREVIEW");
        if (!preview.isEmpty())
            QVERIFY(help.grab().save(preview));
    }
};

QTEST_MAIN(MossLoaderHelpTest)
#include "MossLoaderHelp_test.moc"
