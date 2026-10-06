"""Moss fork regression checks, added 2026-10-02. SPDX-License-Identifier: GPL-3.0-only."""
from pathlib import Path
import re
import unittest

SOURCE = Path(__file__).resolve().parents[1]


class ForkChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cmake = (SOURCE / "CMakeLists.txt").read_text(encoding="utf-8")
        cls.identity = (SOURCE / "program_info/CMakeLists.txt").read_text(encoding="utf-8")

    def test_no_upstream_credentials_or_update_channel(self):
        for variable in ("Launcher_CURSEFORGE_API_KEY", "Launcher_IMGUR_CLIENT_ID",
                         "Launcher_UPDATER_GITHUB_REPO", "MACOSX_SPARKLE_UPDATE_PUBLIC_KEY", "MACOSX_SPARKLE_UPDATE_FEED_URL"):
            self.assertRegex(self.cmake, rf'set\({variable} "" CACHE STRING')

    def test_own_public_client_and_local_oauth(self):
        self.assertIn('set(Launcher_MSA_CLIENT_ID "191f8f9d-bc7c-4f13-8989-ad109a11bf9a"', self.cmake)
        self.assertIn('set(Launcher_LOGIN_CALLBACK_URL ""', self.cmake)
        code = (SOURCE / "launcher/minecraft/auth/steps/MSAStep.cpp").read_text(encoding="utf-8")
        self.assertIn("PkceMethod::S256", code)
        self.assertNotIn("readAll()", code)
        self.assertNotIn("BuildConfig.LOGIN_CALLBACK_URL", code)
        self.assertIn("m_clientId.isEmpty()", code)
        handler = (SOURCE / "launcher/minecraft/auth/MossOAuthReplyHandler.h").read_text(encoding="utf-8")
        self.assertIn('QHostAddress::LocalHost, 0', handler)
        self.assertIn('url.setHost("localhost")', handler)
        self.assertNotIn("window.location", handler)

    def test_independent_config_and_identity(self):
        for text in ('set(Launcher_CommonName "MossPrism")', 'set(Launcher_AppID "local.moss.MossPrism")',
                     'set(Launcher_ENVName "MOSSPRISM" PARENT_SCOPE)',
                     'set(Launcher_ConfigFile "${Launcher_APP_BINARY_NAME}.cfg" PARENT_SCOPE)'):
            self.assertIn(text, self.identity)
        self.assertIn('set(Launcher_APP_BINARY_NAME "mossprism"', self.cmake)

    def test_about_discloses_fork_without_removing_credits(self):
        about = (SOURCE / "launcher/ui/dialogs/AboutDialog.cpp").read_text(encoding="utf-8")
        self.assertIn("not endorsed by or affiliated", about)
        self.assertIn("getCreditsHtml()", about)
        self.assertIn("getLicenseHtml()", about)

    def test_java_compatibility_override_is_preserved(self):
        code = (SOURCE / "launcher/minecraft/launch/VerifyJavaInstall.cpp").read_text(encoding="utf-8")
        self.assertIn('settings->get("IgnoreJavaCompatibility")', code)
        self.assertRegex(code, r"if \(ignoreCompatibility\)\s*\{[\s\S]*?emitSucceeded\(\)")

    def test_provider_and_authentication_code_preserved(self):
        for filename in ("launcher/ui/pages/global/APIPage.cpp", "launcher/modplatform/flame/FlameAPI.cpp",
                         "launcher/modplatform/modrinth/ModrinthAPI.cpp", "launcher/ui/dialogs/MSALoginDialog.cpp"):
            self.assertTrue((SOURCE / filename).is_file(), filename)
        api = (SOURCE / "launcher/ui/pages/global/APIPage.cpp").read_text(encoding="utf-8")
        self.assertIn('s->set("MSAClientIDOverride", msaClientID)', api)
        self.assertIn('s->set("FlameKeyOverride", flameKey)', api)

    def test_license_and_source_notes_present(self):
        self.assertIn("GNU GENERAL PUBLIC LICENSE", (SOURCE / "LICENSE").read_text(encoding="utf-8"))
        notes = (SOURCE / "MOSS-CHANGES.md").read_text(encoding="utf-8")
        self.assertIn("Corresponding Source", notes)
        self.assertIn("7d2d3c1ec6fcf8255ebacd02d9f792688575dfb4", notes)

    def test_login_methods_are_selected_not_started_together(self):
        code = (SOURCE / "launcher/ui/dialogs/MSALoginDialog.cpp").read_text(encoding="utf-8")
        header = (SOURCE / "launcher/ui/dialogs/MSALoginDialog.h").read_text(encoding="utf-8")
        self.assertNotIn("m_devicecode_task", code + header)
        self.assertIn("m_account->login(m_deviceMode)", code)
        self.assertIn("m_attempt.isCurrent(generation)", code)
        self.assertIn("reason.toHtmlEscaped()", code)
        self.assertIn("void MSALoginDialog::done(int result)", code)
        flow = (SOURCE / "launcher/minecraft/auth/AuthFlow.cpp").read_text(encoding="utf-8")
        self.assertNotIn("The session has expired", flow)

    def test_cancelled_callbacks_do_not_modify_old_accounts(self):
        for step in ("XboxUserStep", "XboxAuthorizationStep", "LauncherLoginStep", "EntitlementsStep", "MinecraftProfileStep", "GetSkinStep"):
            code = (SOURCE / f"launcher/minecraft/auth/steps/{step}.cpp").read_text(encoding="utf-8")
            self.assertRegex(code, rf"void {step}::onRequestDone\(QByteArray\* response\)\s*\{{\s*if \(m_cancelled\)\s*return;")

    def test_loader_help_updates_without_changing_filter(self):
        code = (SOURCE / "launcher/ui/dialogs/InstallLoaderDialog.cpp").read_text(encoding="utf-8")
        self.assertIn('setExactIfPresentFilter(BaseVersionList::ParentVersionRole, minecraftVersion)', code)
        self.assertIn('loaderHelp->setContext(this->profile->getComponentVersion("net.minecraft"), current->displayName())', code)
        self.assertIn('loaderHelp->setContext(profile->getComponentVersion("net.minecraft"), container->selectedPage()->displayName())', code)
        self.assertLess(code.index('layout->addWidget(loaderHelp)'), code.index('layout->addWidget(container)'))
        help_code = (SOURCE / "launcher/ui/widgets/MossLoaderHelp.h").read_text(encoding="utf-8")
        self.assertIn('Qt::PlainText', help_code)
        self.assertIn('MODの互換性は保証されません', help_code)

    def test_required_dependencies_need_review_consent(self):
        download = (SOURCE / "launcher/ui/dialogs/ResourceDownloadDialog.cpp").read_text(encoding="utf-8")
        self.assertIn("configureModDependencies(depNames", download)
        self.assertIn('get("ModDependenciesDisabled")', download)
        self.assertLess(download.index("configureModDependencies(depNames"), download.index("confirmDialog->exec()"))
        review = (SOURCE / "launcher/ui/dialogs/ReviewMessageBox.cpp").read_text(encoding="utf-8")
        self.assertIn("一緒に追加しますか", review)
        self.assertIn("warning.setDefaultButton(QMessageBox::No)", review)
        self.assertIn("warning.setTextFormat(Qt::PlainText)", review)
        deps = (SOURCE / "launcher/minecraft/mod/tasks/GetModDependenciesTask.cpp").read_text(encoding="utf-8")
        self.assertIn("ver_dep.type != ModPlatform::DependencyType::REQUIRED", deps)
        self.assertIn("前提MODの対応ファイルが見つかりませんでした", deps)


if __name__ == "__main__":
    unittest.main()
