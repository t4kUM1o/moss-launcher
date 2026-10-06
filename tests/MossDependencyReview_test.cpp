// SPDX-License-Identifier: GPL-3.0-only
// Offline consent checks: synthetic MOD metadata, no downloads or real instances.
#include <QTest>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QSignalSpy>
#include <QTreeWidget>
#include <memory>
#include "ui/dialogs/ReviewMessageBox.h"

class MossDependencyReviewTest : public QObject {
    Q_OBJECT
    using Dialog = std::unique_ptr<ReviewMessageBox>;
    Dialog review()
    {
        auto dialog = Dialog(ReviewMessageBox::create(nullptr, QString("test")));
        dialog->configureModDependencies({ "Sodium", "Already present" }, true);
        dialog->appendResource({ .name = "Iris", .filename = "iris.jar", .provider = "Modrinth" });
        dialog->appendResource({ .name = "Sodium", .filename = "sodium.jar", .provider = "Modrinth", .required_by = { "Iris" } });
        dialog->appendResource({ .name = "Already present", .filename = "existing.jar", .provider = "Modrinth",
                                 .required_by = { "Iris" }, .enabled = false });
        return dialog;
    }
   private slots:
    void initTestCase()
    {
#ifdef Q_OS_WIN
        if (!qEnvironmentVariable("MOSSPRISM_DEPENDENCY_PREVIEW").isEmpty()) {
            const auto id = QFontDatabase::addApplicationFont("C:/Windows/Fonts/meiryo.ttc");
            QVERIFY(id >= 0);
            QApplication::setFont(QFont(QFontDatabase::applicationFontFamilies(id).first(), 9));
        }
#endif
    }
    void dependencyConsentAndLayout()
    {
        auto dialog = review();
        auto* tree = dialog->findChild<QTreeWidget*>("modTreeWidget");
        auto* label = dialog->findChild<QLabel*>("explainLabel");
        QVERIFY(tree && label);
        QCOMPARE(tree->topLevelItemCount(), 3);
        QCOMPARE(tree->topLevelItem(0)->text(1), QStringLiteral("選択したMOD"));
        QCOMPARE(tree->topLevelItem(1)->text(1), QStringLiteral("必須の前提MOD"));
        QCOMPARE(tree->topLevelItem(1)->checkState(0), Qt::Checked);
        QCOMPARE(tree->topLevelItem(2)->checkState(0), Qt::Unchecked);
        QVERIFY(tree->topLevelItem(1)->isExpanded());
        QVERIFY(label->text().contains(QStringLiteral("一緒に追加しますか")));
        QCOMPARE(label->textFormat(), Qt::PlainText);
        QVERIFY(label->wordWrap());
        QCOMPARE(dialog->deselectedResources(), QStringList{ "Already present" });
        dialog->show();
        QVERIFY(QTest::qWaitForWindowExposed(dialog.get()));
        dialog->layout()->activate();
        for (auto* text : dialog->findChildren<QLabel*>())
            if (text->wordWrap())
                QVERIFY(text->height() >= text->heightForWidth(text->width()));
        const auto preview = qEnvironmentVariable("MOSSPRISM_DEPENDENCY_PREVIEW");
        if (!preview.isEmpty())
            QVERIFY(dialog->grab().save(preview));
    }
    void toggleOnlyAutomaticDependencies()
    {
        auto dialog = review();
        // Manually selected MODs that also satisfy a dependency must stay selected.
        dialog->appendResource({ .name = "Manual dependency", .provider = "Modrinth", .required_by = { "Iris" } });
        auto* tree = dialog->findChild<QTreeWidget*>("modTreeWidget");
        auto* toggle = dialog->findChild<QPushButton*>("toggleDepsButton");
        toggle->click();
        QCOMPARE(tree->topLevelItem(0)->checkState(0), Qt::Checked);
        QCOMPARE(tree->topLevelItem(1)->checkState(0), Qt::Unchecked);
        QCOMPARE(tree->topLevelItem(3)->checkState(0), Qt::Checked);
        QCOMPARE(dialog->missingRequiredMods(), (QStringList{ "Sodium", "Already present" }));
        toggle->click();
        QCOMPARE(tree->topLevelItem(1)->checkState(0), Qt::Checked);
        QCOMPARE(tree->topLevelItem(2)->checkState(0), Qt::Unchecked);
        // A deselected parent no longer asks for its dependencies.
        tree->topLevelItem(0)->setCheckState(0, Qt::Unchecked);
        QVERIFY(dialog->missingRequiredMods().isEmpty());
    }
    void warningDefaultsToGoingBack_data()
    {
        QTest::addColumn<bool>("proceed");
        QTest::newRow("go-back") << false;
        QTest::newRow("explicit-proceed") << true;
    }
    void warningDefaultsToGoingBack()
    {
        QFETCH(bool, proceed);
        auto dialog = review();
        auto* tree = dialog->findChild<QTreeWidget*>("modTreeWidget");
        tree->topLevelItem(1)->setCheckState(0, Qt::Unchecked);
        QSignalSpy accepted(dialog.get(), &QDialog::accepted);
        QTimer::singleShot(0, dialog.get(), [&]() {
            auto* warning = dialog->findChild<QMessageBox*>();
            QVERIFY(warning);
            QCOMPARE(warning->textFormat(), Qt::PlainText);
            QCOMPARE(warning->defaultButton(), warning->button(QMessageBox::No));
            QVERIFY(warning->text().contains("Sodium"));
            warning->button(proceed ? QMessageBox::Yes : QMessageBox::No)->click();
        });
        dialog->findChild<QDialogButtonBox*>("buttonBox")->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(accepted.count(), proceed ? 1 : 0);
        QCOMPARE(tree->topLevelItem(1)->checkState(0), Qt::Unchecked);
    }
    void completeSelectionAndCancel()
    {
        auto dialog = review();
        auto* tree = dialog->findChild<QTreeWidget*>("modTreeWidget");
        tree->topLevelItem(2)->setCheckState(0, Qt::Checked);
        QSignalSpy accepted(dialog.get(), &QDialog::accepted);
        dialog->findChild<QDialogButtonBox*>("buttonBox")->button(QDialogButtonBox::Ok)->click();
        QCOMPARE(accepted.count(), 1);
        QVERIFY(!dialog->findChild<QMessageBox*>());
        auto cancelled = review();
        QSignalSpy rejected(cancelled.get(), &QDialog::rejected);
        cancelled->findChild<QDialogButtonBox*>("buttonBox")->button(QDialogButtonBox::Cancel)->click();
        QCOMPARE(rejected.count(), 1);
    }
    void disabledOrIncompleteLookupAndOtherResources()
    {
        auto dialog = Dialog(ReviewMessageBox::create(nullptr, QString("test")));
        dialog->configureModDependencies({}, false);
        auto* label = dialog->findChild<QLabel*>("explainLabel");
        QVERIFY(label->text().contains(QStringLiteral("無効")));
        dialog->configureModDependencies({}, true, false);
        QVERIFY(label->text().contains(QStringLiteral("すべて揃っているとは限りません")));
        dialog->configureModDependencies({}, true);
        QVERIFY(label->text().contains(QStringLiteral("見つかりませんでした")));
        auto generic = Dialog(ReviewMessageBox::create(nullptr, QString("test")));
        generic->retranslateUi("shader packs");
        generic->appendResource({ .name = "Shader pack" });
        QVERIFY(!generic->property("mossModDependencyReview").toBool());
        QVERIFY(!generic->findChild<QPushButton*>("toggleDepsButton")->isVisible());
        QCOMPARE(generic->findChild<QTreeWidget*>("modTreeWidget")->topLevelItem(0)->text(1), QString());
    }
    void requiredDependencyWithoutParentEdge()
    {
        auto dialog = Dialog(ReviewMessageBox::create(nullptr, QString("test")));
        dialog->configureModDependencies({ "API" }, true);
        dialog->appendResource({ .name = "Root MOD" });
        dialog->appendResource({ .name = "API" });
        auto* tree = dialog->findChild<QTreeWidget*>("modTreeWidget");
        dialog->findChild<QPushButton*>("toggleDepsButton")->click();
        QCOMPARE(tree->topLevelItem(0)->checkState(0), Qt::Checked);
        QCOMPARE(dialog->missingRequiredMods(), QStringList{ "API" });
    }
};

QTEST_MAIN(MossDependencyReviewTest)
#include "MossDependencyReview_test.moc"
