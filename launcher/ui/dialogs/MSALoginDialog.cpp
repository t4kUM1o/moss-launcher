// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "MSALoginDialog.h"
#include "Application.h"
#include "settings/SettingsObject.h"

#include "ui_MSALoginDialog.h"

#include "DesktopServices.h"
#include "minecraft/auth/AuthFlow.h"

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QUrl>
#include <QtWidgets/QPushButton>

#include "qrencode.h"

MSALoginDialog::MSALoginDialog(QWidget* parent) : QDialog(parent), ui(new Ui::MSALoginDialog)
{
    ui->setupUi(this);

    auto methodRow = new QHBoxLayout;
    methodRow->addWidget(new QLabel(tr("ログイン方式"), this));
    m_method = new QComboBox(this);
    m_method->addItem(tr("ブラウザでログイン（推奨）"));
    m_method->addItem(tr("コード入力でログイン"));
    methodRow->addWidget(m_method, 1);
    ui->verticalLayout_6->insertLayout(0, methodRow);
    ui->line_3->hide();
    ui->line_4->hide();
    ui->orLabel->hide();
    m_retry = ui->buttonBox->addButton(tr("再試行"), QDialogButtonBox::ActionRole);
    m_retry->setEnabled(false);
    connect(m_retry, &QPushButton::clicked, this, &MSALoginDialog::startLogin);
    connect(m_method, &QComboBox::currentIndexChanged, this, [this] { startLogin(); });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // make font monospace
    QFont font;
    font.setPixelSize(ui->code->fontInfo().pixelSize());
    font.setFamily(APPLICATION->settings()->get("ConsoleFont").toString());
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    ui->code->setFont(font);

    connect(ui->copyCode, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(ui->code->text()); });
    connect(ui->loginButton, &QPushButton::clicked, this, [this] {
        if (m_url.isValid()) {
            if (!DesktopServices::openUrl(m_url)) {
                QApplication::clipboard()->setText(m_url.toString());
            }
        }
    });

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

int MSALoginDialog::exec()
{
    startLogin();
    return QDialog::exec();
}

void MSALoginDialog::stopLogin()
{
    // Keep the old account's raw AccountData alive until its deferred task disposal.
    if (auto task = m_attempt.task()) {
        const auto account = m_account;
        connect(task, &QObject::destroyed, this, [account] {});
    }
    m_attempt.cancel();
}

void MSALoginDialog::startLogin()
{
    stopLogin();
    m_url = QUrl();
    ui->loginButton->setToolTip(QString());
    ui->loginButton->setEnabled(false);
    ui->code->clear();
    ui->qr->clear();
    ui->qrMessage->clear();
    m_retry->setEnabled(false);
    m_deviceMode = m_method->currentIndex() == 1;
    ui->stackedWidget2->setVisible(!m_deviceMode);
    ui->stackedWidget->setVisible(m_deviceMode);
    ui->stackedWidget2->setCurrentIndex(0);
    ui->stackedWidget->setCurrentIndex(0);
    ui->loadingLabel2->setText(tr("ブラウザ方式でログイン"));
    ui->loadingLabel->setText(tr("コード入力方式でログイン"));
    ui->status2->setText(tr("ログインを準備しています…"));
    ui->status->setText(tr("ログインを準備しています…"));
    m_account = MinecraftAccount::createBlankMSA();
    const auto flow = m_account->login(m_deviceMode);
    const auto generation = m_attempt.replace(flow);
    connect(flow.get(), &Task::failed, &m_attempt, [this, generation](const QString& reason) {
        if (m_attempt.isCurrent(generation))
            onTaskFailed(reason);
    });
    connect(flow.get(), &Task::succeeded, &m_attempt, [this, generation] {
        if (m_attempt.isCurrent(generation))
            accept();
    });
    connect(flow.get(), &Task::status, &m_attempt, [this, generation](const QString& status) {
        if (!m_attempt.isCurrent(generation))
            return;
        if (m_deviceMode)
            onDeviceFlowStatus(status);
        else
            onAuthFlowStatus(status);
    });
    connect(flow.get(), &AuthFlow::authorizeWithBrowser, &m_attempt, [this, generation](const QUrl& url) {
        if (m_attempt.isCurrent(generation))
            authorizeWithBrowser(url);
    });
    connect(flow.get(), &AuthFlow::authorizeWithBrowserWithExtra, &m_attempt,
            [this, generation](const QString& url, const QString& code, int expiresIn) {
                if (m_attempt.isCurrent(generation))
                    authorizeWithBrowserWithExtra(url, code, expiresIn);
            });
}

void MSALoginDialog::done(int result)
{
    stopLogin();
    QDialog::done(result);
}

MSALoginDialog::~MSALoginDialog()
{
    stopLogin();
    delete ui;
}

void MSALoginDialog::onTaskFailed(QString reason)
{
    auto pane = m_deviceMode ? ui->stackedWidget : ui->stackedWidget2;
    auto title = m_deviceMode ? ui->loadingLabel : ui->loadingLabel2;
    auto details = m_deviceMode ? ui->status : ui->status2;
    pane->setCurrentIndex(0);
    title->setText(tr("ログインに失敗しました"));
    details->setText("<font color='red'>" + reason.toHtmlEscaped().replace('\n', "<br />") + "</font>");
    m_retry->setEnabled(true);
    m_url = QUrl();
    ui->loginButton->setEnabled(false);
    ui->loginButton->setToolTip(QString());
}

void MSALoginDialog::authorizeWithBrowser(const QUrl& url)
{
    ui->stackedWidget2->setCurrentIndex(1);
    ui->stackedWidget2->adjustSize();
    ui->stackedWidget2->updateGeometry();
    this->adjustSize();
    ui->loginButton->setToolTip(QString("<div style='width: 200px;'>%1</div>").arg(url.toString()));
    m_url = url;
    ui->loginButton->setEnabled(true);
}

void paintQR(QPainter& painter, const QSize canvasSize, const QString& data, QColor fg)
{
    const auto* qr = QRcode_encodeString(data.toUtf8().constData(), 0, QRecLevel::QR_ECLEVEL_M, QRencodeMode::QR_MODE_8, 1);
    if (!qr) {
        qWarning() << "Unable to encode login QR code"; // Never log a login URL or one-time code.
        return;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(fg);

    // Make sure the QR code fits in the canvas with some padding
    const auto qrSize = qr->width;
    const auto canvasWidth = canvasSize.width();
    const auto canvasHeight = canvasSize.height();
    const auto scale = 0.8 * std::min(canvasWidth / qrSize, canvasHeight / qrSize);

    // Find an offset to center it in the canvas
    const auto offsetX = (canvasWidth - qrSize * scale) / 2;
    const auto offsetY = (canvasHeight - qrSize * scale) / 2;

    for (int y = 0; y < qrSize; y++) {
        for (int x = 0; x < qrSize; x++) {
            auto shouldFillIn = qr->data[y * qrSize + x] & 1;
            if (shouldFillIn) {
                QRectF r(offsetX + x * scale, offsetY + y * scale, scale, scale);
                painter.drawRects(&r, 1);
            }
        }
    }
}

void MSALoginDialog::authorizeWithBrowserWithExtra(QString url, QString code, [[maybe_unused]] int expiresIn)
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->stackedWidget->adjustSize();
    ui->stackedWidget->updateGeometry();
    this->adjustSize();

    const auto linkString = QString("<a href=\"%1\">%2</a>").arg(url, url);
    if (url == "https://www.microsoft.com/link" && !code.isEmpty()) {
        url += QString("?otc=%1").arg(code);
    }
    ui->code->setText(code);

    auto size = QSize(150, 150);
    QPixmap pixmap(size);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    paintQR(painter, size, url, Qt::black);

    // Set the generated pixmap to the label
    ui->qr->setPixmap(pixmap);

    ui->qrMessage->setText(tr("Open %1 or scan the QR and enter the above code if needed.").arg(linkString));
}

void MSALoginDialog::onDeviceFlowStatus(QString status)
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget->adjustSize();
    ui->stackedWidget->updateGeometry();
    this->adjustSize();
    ui->status->setText(status);
}

void MSALoginDialog::onAuthFlowStatus(QString status)
{
    ui->stackedWidget2->setCurrentIndex(0);
    ui->stackedWidget2->adjustSize();
    ui->stackedWidget2->updateGeometry();
    this->adjustSize();
    ui->status2->setText(status);
}

// Public interface
MinecraftAccountPtr MSALoginDialog::newAccount(QWidget* parent)
{
    MSALoginDialog dlg(parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.m_account;
    }
    return nullptr;
}
