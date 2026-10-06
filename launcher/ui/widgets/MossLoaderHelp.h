// SPDX-License-Identifier: GPL-3.0-only
// Moss modification, 2026-10-03: explain the loader version list in context.
#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class MossLoaderHelp final : public QWidget {
   public:
    explicit MossLoaderHelp(QWidget* parent = nullptr) : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 8, 12, 4);
        layout->setSpacing(4);
        m_target = new QLabel(this);
        m_versions = new QLabel(this);
        m_mods = new QLabel(this);
        m_target->setObjectName("mossLoaderTarget");
        m_versions->setObjectName("mossLoaderVersions");
        m_mods->setObjectName("mossLoaderModCompatibility");
        auto targetFont = m_target->font();
        targetFont.setBold(true);
        m_target->setFont(targetFont);
        for (auto* label : { m_target, m_versions, m_mods }) {
            label->setTextFormat(Qt::PlainText);
            label->setWordWrap(true);
            label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            layout->addWidget(label);
        }
        setToolTip(tr("候補は配信元の対応情報に基づきます。複数のMinecraftバージョンで共通のローダーは、"
                      "同じ候補が表示される場合があります。ゲームやMODの起動を保証するものではありません。"));
    }

    void setContext(const QString& minecraftVersion, const QString& loaderName)
    {
        const auto version = minecraftVersion.isEmpty() ? tr("未設定") : minecraftVersion;
        m_target->setText(tr("対象：Minecraft %1 ／ %2").arg(version, loaderName));
        m_versions->setText(tr("一覧の数字は%1自身のバージョンです（Minecraftのバージョンではありません）。"
                               "起動構成のMinecraftに合わせた候補を表示しています。")
                                .arg(loaderName));
        m_mods->setText(tr("MODは「Minecraft %1・%2対応」のものを選んでください。"
                           "ローダーを導入するだけでは、MODの互換性は保証されません。")
                            .arg(version, loaderName));
    }

   private:
    QLabel* m_target;
    QLabel* m_versions;
    QLabel* m_mods;
};
