"""Offline beta packaging invariants. SPDX-License-Identifier: GPL-3.0-only."""
import importlib.util
from pathlib import Path
import unittest

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("beta_packager", HERE / "build-beta-installer.py")
packager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packager)


class PackagingChecks(unittest.TestCase):
    def test_safe_names(self):
        for name in ("mossprism.exe", "platforms/qwindows.dll", "licenses/qtbase/LGPL-3.0-only.txt", "Source.zip"):
            self.assertEqual(packager.safe_name(name).name, Path(name).name)

    def test_private_and_escaping_paths(self):
        for name in ("UserData/accounts.json", "accounts.json", "mossprism.cfg", "portable.txt", ".env.private",
                     "private.key", "../elsewhere", "C:/elsewhere", "C:relative", "secrets/key.txt", "file$INJECT.dll"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                packager.safe_name(name)

    def test_no_admin_or_forced_kill_or_recursive_deletion(self):
        code = "\n".join(line for line in (HERE / "beta.nsi").read_text(encoding="utf-8").splitlines()
                         if not line.lstrip().startswith(";"))
        self.assertIn("RequestExecutionLevel user", code)
        self.assertIn("SetShellVarContext current", code)
        self.assertNotIn("RMDir /r", code)
        self.assertNotIn("TaskKill", code)
        self.assertNotIn("HKLM", code)
        self.assertNotIn("File /r", code)
        self.assertNotIn("DeleteRegKey HKCU Software\\Classes", code)

    def test_installed_data_retained(self):
        code = (HERE / "beta.nsi").read_text(encoding="utf-8")
        uninstall = code.split('Section "Uninstall"', 1)[1]
        self.assertNotIn('Delete "$INSTDIR\\UserData', uninstall)
        self.assertNotIn('RMDir "$INSTDIR\\UserData', uninstall)
        self.assertNotIn('RMDir "$INSTDIR"', uninstall)
        self.assertIn("CHECK_EXE_LOCK", code)
        self.assertIn("CHECK_REPARSE", code)
        self.assertIn("GetFullPathNameW", code)
        self.assertIn('ReadINIStr $0 "$INSTDIR\\${INSTALL_MARKER}" "Moss" "AppID"', code)

    def test_only_manifest_data_is_packaged(self):
        code = (HERE / "build-beta-installer.py").read_text(encoding="utf-8")
        self.assertIn('original = json.loads((runtime / "SHA256.json")', code)
        self.assertNotIn('runtime.rglob', code)
        self.assertIn('encoding="utf-8-sig"', code)
        self.assertIn('"/INPUTCHARSET", "UTF8"', code)
        self.assertIn('packager.pack(refresh_source=True)', code)


if __name__ == "__main__":
    unittest.main()
