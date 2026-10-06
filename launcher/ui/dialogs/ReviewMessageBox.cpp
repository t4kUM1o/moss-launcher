#include "ReviewMessageBox.h"
#include "ui_ReviewMessageBox.h"

#include <QClipboard>
#include <QPushButton>
#include <QShortcut>
#include <QMessageBox>
#include <QSet>

ReviewMessageBox::ReviewMessageBox(QWidget* parent, [[maybe_unused]] QString const& title, [[maybe_unused]] QString const& icon)
    : QDialog(parent), ui(new Ui::ReviewMessageBox)
{
    ui->setupUi(this);

    auto back_button = ui->buttonBox->button(QDialogButtonBox::Cancel);
    back_button->setText(tr("Back"));

    ui->toggleDepsButton->hide();
    ui->modTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->modTreeWidget->header()->setStretchLastSection(false);
    ui->modTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ReviewMessageBox::confirmSelection);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ReviewMessageBox::reject);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    // Overwrite Ctrl+C functionality to exclude the label when copying text from tree
    auto shortcut = new QShortcut(QKeySequence::Copy, ui->modTreeWidget);
    connect(shortcut, &QShortcut::activated, [this]() {
        auto currentItem = this->ui->modTreeWidget->currentItem();
        if (!currentItem)
            return;
        auto currentColumn = this->ui->modTreeWidget->currentColumn();

        auto data = currentItem->data(currentColumn, Qt::UserRole);
        QString txt;

        if (data.isValid()) {
            txt = data.toString();
        } else {
            txt = currentItem->text(currentColumn);
        }

        QApplication::clipboard()->setText(txt);
    });
}

ReviewMessageBox::~ReviewMessageBox()
{
    delete ui;
}

auto ReviewMessageBox::create(QWidget* parent, QString&& title, QString&& icon) -> ReviewMessageBox*
{
    return new ReviewMessageBox(parent, title, icon);
}

void ReviewMessageBox::appendResource(ResourceInformation&& info)
{
    auto itemTop = new QTreeWidgetItem(ui->modTreeWidget);
    itemTop->setCheckState(0, info.enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    itemTop->setText(0, info.name);
    const bool modReview = property("mossModDependencyReview").toBool();
    const bool automaticDependency = property("mossDependencyNames").toStringList().contains(info.name);
    itemTop->setData(0, Qt::UserRole + 2, info.required_by);
    itemTop->setData(0, Qt::UserRole + 3, info.enabled);
    itemTop->setData(0, Qt::UserRole + 4, automaticDependency);
    if (modReview) {
        itemTop->setText(1, automaticDependency ? QStringLiteral("必須の前提MOD") : QStringLiteral("選択したMOD"));
        if (!info.enabled)
            itemTop->setText(1, QStringLiteral("導入済みの可能性"));
    }
    if (!info.enabled) {
        itemTop->setToolTip(0, modReview ? QStringLiteral("同じMODが導入済みの可能性があります。既存のバージョンを確認してから選択してください。")
                                       : tr("Mod was disabled as it may be already installed."));
    }

    auto filenameItem = new QTreeWidgetItem(itemTop);
    filenameItem->setText(0, (modReview ? QStringLiteral("ファイル：%1") : tr("Filename: %1")).arg(info.filename));
    filenameItem->setData(0, Qt::UserRole, info.filename);

    auto providerItem = new QTreeWidgetItem(itemTop);
    providerItem->setText(0, (modReview ? QStringLiteral("配布元：%1") : tr("Provider: %1")).arg(info.provider));
    providerItem->setData(0, Qt::UserRole, info.provider);

    if (!info.required_by.isEmpty() || (modReview && automaticDependency)) {
        auto requiredByItem = new QTreeWidgetItem(itemTop);
        if (info.required_by.length() == 1) {
            requiredByItem->setText(0, (modReview ? QStringLiteral("必要としているMOD：%1") : tr("Required by: %1"))
                                          .arg(info.required_by.back()));
            requiredByItem->setData(0, Qt::UserRole, info.required_by.back());
        } else if (!info.required_by.isEmpty()) {
            requiredByItem->setText(0, modReview ? QStringLiteral("必要としているMOD：") : tr("Required by:"));
            for (auto req : info.required_by) {
                auto reqItem = new QTreeWidgetItem(requiredByItem);
                reqItem->setText(0, req);
            }
        } else {
            requiredByItem->setText(0, QStringLiteral("配布元の情報で必須と指定されている前提MODです。"));
        }

        if (!modReview || automaticDependency) {
            ui->toggleDepsButton->show();
            m_deps << itemTop;
        }
        if (modReview)
            itemTop->setExpanded(true);
    }

    auto versionTypeItem = new QTreeWidgetItem(itemTop);
    versionTypeItem->setText(0, (modReview ? QStringLiteral("公開種別：%1") : tr("Version Type: %1")).arg(info.version_type));
    versionTypeItem->setData(0, Qt::UserRole, info.version_type);

    ui->modTreeWidget->addTopLevelItem(itemTop);
}

void ReviewMessageBox::configureModDependencies(const QStringList& dependencyNames, bool checkEnabled, bool lookupComplete)
{
    setProperty("mossModDependencyReview", true);
    setProperty("mossDependencyNames", dependencyNames);
    setWindowTitle(QStringLiteral("MODと前提MODの確認"));
    ui->explainLabel->setTextFormat(Qt::PlainText);
    ui->explainLabel->setWordWrap(true);
    if (!checkEnabled) {
        ui->explainLabel->setText(QStringLiteral("前提MODの自動確認が設定で無効になっています。\n"
                                                "必要な前提MODが導入済みか確認してから進んでください。"));
    } else if (!lookupComplete) {
        ui->explainLabel->setText(QStringLiteral("前提MODの確認に警告があります。必要な候補がすべて揃っているとは限りません。\n"
                                                "配布元の依存情報と、直前の警告を確認してから進んでください。"));
    } else if (dependencyNames.isEmpty()) {
        ui->explainLabel->setText(QStringLiteral("選択したMODを導入しますか？\n"
                                                "配布元の情報から、追加が必要な前提MODは見つかりませんでした。"));
    } else {
        ui->explainLabel->setText(QStringLiteral("必須の前提MODが %1 件見つかりました。一緒に追加しますか？\n"
                                                "起動構成のMinecraft版・ローダーに合わせて候補を確認しています。")
                                     .arg(dependencyNames.size()));
    }
    ui->onlyCheckedLabel->setTextFormat(Qt::PlainText);
    ui->onlyCheckedLabel->setWordWrap(true);
    ui->onlyCheckedLabel->setText(QStringLiteral("チェックしたMODだけを導入します。前提MODを外すと起動できない場合があります。\n"
                                               "確認は配布元の必須依存情報に基づきます。任意の追加MODは自動追加しません。"));
    ui->toggleDepsButton->setText(QStringLiteral("前提MODを外す"));
    ui->toggleDepsButton->setToolTip(QStringLiteral("今回自動で見つかった前提MODだけを切り替えます。自分で選んだMODは変更しません。"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("戻る"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("この選択で導入"));
}

QStringList ReviewMessageBox::missingRequiredMods() const
{
    QSet<QString> checkedNames;
    bool hasSelectedRoot = false;
    for (int i = 0; i < ui->modTreeWidget->topLevelItemCount(); ++i) {
        auto* item = ui->modTreeWidget->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            checkedNames.insert(item->text(0));
            hasSelectedRoot |= !item->data(0, Qt::UserRole + 4).toBool();
        }
    }
    QStringList missing;
    for (int i = 0; i < ui->modTreeWidget->topLevelItemCount(); ++i) {
        auto* item = ui->modTreeWidget->topLevelItem(i);
        if (item->checkState(0) != Qt::Unchecked)
            continue;
        const auto requiredByList = item->data(0, Qt::UserRole + 2).toStringList();
        // Loader overrides can change a project's ID, losing its required-by
        // edge. It is still a REQUIRED dependency, so don't omit the warning.
        if (requiredByList.isEmpty() && hasSelectedRoot && item->data(0, Qt::UserRole + 4).toBool()) {
            missing.append(item->text(0));
            continue;
        }
        for (const auto& requiredBy : requiredByList) {
            if (checkedNames.contains(requiredBy)) {
                missing.append(item->text(0));
                break;
            }
        }
    }
    return missing;
}

void ReviewMessageBox::confirmSelection()
{
    if (property("mossModDependencyReview").toBool()) {
        const auto missing = missingRequiredMods();
        if (!missing.isEmpty()) {
            QMessageBox warning(QMessageBox::Warning, QStringLiteral("前提MODが選択されていません"),
                                QStringLiteral("以下の前提MODが未選択です：\n%1\n\n"
                                               "既に導入済みでなければ、選んだMODが動かない・起動できない場合があります。\n"
                                               "前提MODを追加せずに進みますか？")
                                    .arg(missing.join('\n')),
                                QMessageBox::Yes | QMessageBox::No, this);
            warning.setTextFormat(Qt::PlainText);
            warning.button(QMessageBox::Yes)->setText(QStringLiteral("追加せずに進む"));
            warning.button(QMessageBox::No)->setText(QStringLiteral("戻って選び直す"));
            warning.setDefaultButton(QMessageBox::No);
            if (warning.exec() != QMessageBox::Yes)
                return;
        }
    }
    QDialog::accept();
}

auto ReviewMessageBox::deselectedResources() -> QStringList
{
    QStringList list;

    auto* item = ui->modTreeWidget->topLevelItem(0);

    for (int i = 1; item != nullptr; ++i) {
        if (item->checkState(0) == Qt::CheckState::Unchecked) {
            list.append(item->text(0));
        }

        item = ui->modTreeWidget->topLevelItem(i);
    }

    return list;
}

void ReviewMessageBox::retranslateUi(QString resources_name)
{
    setWindowTitle(tr("Confirm %1 selection").arg(resources_name));

    ui->explainLabel->setText(tr("You're about to download the following %1:").arg(resources_name));
    ui->onlyCheckedLabel->setText(tr("Only %1 with a check will be downloaded!").arg(resources_name));
}
void ReviewMessageBox::on_toggleDepsButton_clicked()
{
    m_deps_checked = !m_deps_checked;
    auto state = m_deps_checked ? Qt::Checked : Qt::Unchecked;
    for (auto dep : m_deps) {
        // A filename-only match is uncertain: don't silently reselect it.
        if (property("mossModDependencyReview").toBool() && state == Qt::Checked && !dep->data(0, Qt::UserRole + 3).toBool())
            continue;
        dep->setCheckState(0, state);
    }
    if (property("mossModDependencyReview").toBool())
        ui->toggleDepsButton->setText(m_deps_checked ? QStringLiteral("前提MODを外す") : QStringLiteral("前提MODを選択"));
};
