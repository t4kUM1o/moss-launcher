"""Manifest-only beta installer build. SPDX-License-Identifier: GPL-3.0-only."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path, PureWindowsPath
import shutil
import subprocess
import zipfile

SOURCE = Path(__file__).resolve().parents[1]
VERSION = "0.1.0-beta.1"


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def safe_name(name):
    path = PureWindowsPath(name)
    if path.is_absolute() or path.drive or ".." in path.parts or any(c in name for c in '$"\r\n'):
        raise ValueError(f"Unsafe payload path: {name}")
    if any(p.lower() in {"userdata", ".git", "secrets", "work", "outputs"} for p in path.parts):
        raise ValueError(f"Private payload path: {name}")
    if path.name.lower() in {"accounts.json", "mossprism.cfg", "prismlauncher.cfg", ".env", "portable.txt"}:
        raise ValueError(f"Personal/portable file: {name}")
    if path.name.lower().startswith(".env.") or path.suffix.lower() in {".pfx", ".p12", ".key", ".pem", ".pyc"}:
        raise ValueError(f"Private credential/cache file: {name}")
    return path


def build(runtime, nsis, output):
    runtime, nsis, output = runtime.resolve(), nsis.resolve(), output.resolve()
    if output.exists():
        raise RuntimeError("Use a new output directory; old packages must not be overwritten.")
    if not (nsis / "makensis.exe").is_file():
        raise RuntimeError("NSIS compiler not found.")
    original = json.loads((runtime / "SHA256.json").read_text(encoding="utf-8"))
    # Never enumerate/copy runtime UserData, even if it contains real accounts.
    for name, expected in original.items():
        target = runtime / name
        if not target.resolve().is_relative_to(runtime) or target.is_symlink():
            raise RuntimeError("Runtime manifest escaped source folder.")
        if sha(target) != expected:
            raise RuntimeError(f"Runtime manifest mismatch: {name}")
        if name != "portable.txt":
            safe_name(name)
    output.mkdir(parents=True)
    payload = output / "payload"
    generated = output / "generated"
    payload.mkdir()
    generated.mkdir()
    for name in original:
        if name == "portable.txt":
            continue
        target = payload / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(runtime / name, target)
    # BOM makes Japanese license-page decoding explicit on non-UTF-8 Windows.
    (payload / "README-BETA.txt").write_text((SOURCE / "installer/README-BETA.txt").read_text(encoding="utf-8"), encoding="utf-8-sig")
    shutil.copy2(SOURCE / "installer/INSTALLER-VERIFICATION.txt", payload / "INSTALLER-VERIFICATION.txt")
    shutil.copy2(nsis / "COPYING", payload / "NSIS-NOTICE.txt")
    (payload / "BETA-VERSION.json").write_text(json.dumps({
        "version": VERSION, "channel": "beta", "created": "2026-10-06",
        "upstream": "Prism Launcher 11.1.1", "launcher_exe_sha256": original["mossprism.exe"],
        "unsigned": True, "minecraft_login_and_launch_verified": False,
        "minecraft_api_approval_confirmed": False,
    }, indent=2), encoding="utf-8")
    spec = importlib.util.spec_from_file_location("moss_packager", SOURCE / "pack-moss-release.py")
    packager = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(packager)
    packager.RELEASE = payload
    packager.pack(refresh_source=True)
    manifest = json.loads((payload / "SHA256.json").read_text(encoding="utf-8"))
    names = sorted([*manifest, "SHA256.json"])
    install, remove, check = [], [], []
    directories = set()
    for name in names:
        path = safe_name(name)
        parent = str(path.parent)
        suffix = "" if parent == "." else "\\" + parent
        install.extend([f'SetOutPath "$INSTDIR{suffix}"', f'File "${{PAYLOAD_ROOT}}\\{path}"'])
        remove.append(f'Delete "$INSTDIR\\{path}"')
        check.append(f'!insertmacro CHECK_REPARSE "$INSTDIR\\{path}"')
        for directory in path.parents:
            if str(directory) != ".":
                directories.add(str(directory))
    for directory in sorted(directories):
        check.insert(0, f'!insertmacro CHECK_REPARSE "$INSTDIR\\{directory}"')
    for directory in sorted(directories, key=lambda item: (-len(PureWindowsPath(item).parts), item)):
        remove.append(f'RMDir "$INSTDIR\\{directory}"')
    for filename, lines in (("install-files.nsh", install), ("uninstall-files.nsh", remove), ("check-paths.nsh", check)):
        (generated / filename).write_text("\n".join(lines) + "\n", encoding="utf-8")
    setup = output / f"MossLauncher-{VERSION}-windows-x64-setup.exe"
    command = [str(nsis / "makensis.exe"), "/NOCONFIG", "/INPUTCHARSET", "UTF8", "/V3", "/WX",
               f"/DBETA_VERSION={VERSION}", f"/DSOURCE_ROOT={SOURCE}", f"/DPAYLOAD_ROOT={payload}",
               f"/DGENERATED_ROOT={generated}", f"/DSETUP_OUT={setup}",
               f"/DPAYLOAD_KIB={(sum((payload / name).stat().st_size for name in names) + 1023) // 1024}",
               str(SOURCE / "installer/beta.nsi")]
    subprocess.run(command, check=True)
    with zipfile.ZipFile(payload / "Source.zip") as archive:
        if archive.testzip() is not None:
            raise RuntimeError("Launcher source CRC failure.")
        for file in ("installer/beta.nsi", "installer/build-beta-installer.py", "installer/README-BETA.txt",
                     "installer/test-beta-installer.py", "installer/test-beta-packaging.py", "installer/INSTALLER-VERIFICATION.txt"):
            if archive.read("moss-prism/" + file) != (SOURCE / file).read_bytes():
                raise RuntimeError("Installer source mismatch.")
    checksums = {setup.name: sha(setup)}
    (output / "SHA256.json").write_text(json.dumps(checksums, indent=2), encoding="utf-8")
    shutil.copy2(payload / "README-BETA.txt", output / "README-BETA.txt")
    print(json.dumps({"setup": str(setup), "sha256": checksums[setup.name], "bytes": setup.stat().st_size,
                      "payload_files": len(names), "personal_data_included": False}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--nsis", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build(args.runtime, args.nsis, args.output)
