"""Isolated silent install/upgrade/uninstall checks; no real data or integration.
SPDX-License-Identifier: GPL-3.0-only
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess
import time
import winreg


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def registry_snapshot():
    result = {}
    for name in (r"Software\MossLauncherBeta", r"Software\Microsoft\Windows\CurrentVersion\Uninstall\MossLauncherBeta"):
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, name, 0, winreg.KEY_READ | winreg.KEY_WOW64_64KEY) as key:
                values = []
                for i in range(winreg.QueryInfoKey(key)[1]):
                    values.append(winreg.EnumValue(key, i))
                result[name] = sorted(values)
        except FileNotFoundError:
            result[name] = None
    return result


def run_silent(command, timeout=55):
    info = subprocess.STARTUPINFO()
    info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    info.wShowWindow = 0
    return subprocess.run(command, startupinfo=info, timeout=timeout).returncode


def main(setup, payload, qa):
    setup, payload, qa = setup.resolve(), payload.resolve(), qa.resolve()
    workspace = Path(__file__).resolve().parents[2]
    if not qa.is_relative_to(workspace / "work") or qa.exists():
        raise RuntimeError("Use a new, isolated QA directory within workspace/work.")
    qa.mkdir(parents=True)
    target = qa / "日本語 保存先"
    registry_before = registry_snapshot()
    manifest = json.loads((payload / "SHA256.json").read_text(encoding="utf-8"))

    def install(folder):
        # /D must be the last, unquoted argument for NSIS, including Unicode/spaces.
        return run_silent(f'"{setup}" /S /NoIntegration /D={folder}')

    def verify():
        for name, expected in manifest.items():
            assert digest(target / name) == expected, name
        assert (target / "UserData").is_dir()
        assert not (target / "portable.txt").exists()
        assert registry_snapshot() == registry_before

    initial_code = install(target)
    assert initial_code == 0, f"initial install exit code: {initial_code}"
    verify()
    assert not any((target / "UserData").iterdir())
    test_files = {
        "UserData/mossprism.cfg": b"synthetic beta QA configuration; not a real profile\n",
        "UserData/instances/qa/.minecraft/saves/qa-world/level.dat": b"synthetic world preservation test\n",
        "UserData/instances/qa/.minecraft/mods/qa.jar": b"synthetic MOD preservation test\n",
        "user-note.txt": b"unmanaged files must not be removed\n",
    }
    for name, content in test_files.items():
        path = target / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
    assert install(target) == 0, "upgrade install"
    verify()
    for name, content in test_files.items():
        assert (target / name).read_bytes() == content, name
    # Refuse to write over an unrelated non-empty folder.
    unrelated = qa / "unrelated"
    unrelated.mkdir()
    (unrelated / "keep.txt").write_bytes(b"must not change")
    assert install(unrelated) == 104
    assert sorted(path.name for path in unrelated.iterdir()) == ["keep.txt"]
    assert (unrelated / "keep.txt").read_bytes() == b"must not change"
    # Simulate a running/locked executable without launching the launcher.
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p,
                                  ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
    kernel.CreateFileW.restype = ctypes.c_void_p
    kernel.CloseHandle.argtypes = [ctypes.c_void_p]
    handle = kernel.CreateFileW(str(target / "mossprism.exe"), 0x80000000, 0, None, 3, 0, None)
    assert handle != ctypes.c_void_p(-1).value
    try:
        assert install(target) == 105, "locked executable must not be replaced"
        assert run_silent(f'"{target / "uninstall.exe"}" /S _?={target}') == 105, "locked launcher must not be uninstalled"
    finally:
        kernel.CloseHandle(handle)
    verify()
    # Use only the exact isolated install folder. Never call this on a real install.
    code = run_silent(f'"{target / "uninstall.exe"}" /S _?={target}')
    assert code == 0, "uninstall"
    deadline = time.monotonic() + 5
    while (target / "mossprism.exe").exists() and time.monotonic() < deadline:
        time.sleep(0.1)
    for name in manifest:
        assert not (target / name).exists(), name
    for name, content in test_files.items():
        assert (target / name).read_bytes() == content, name
    assert (target / "moss-beta-install.ini").is_file()
    assert registry_snapshot() == registry_before
    assert install(target) == 0, "reinstall into retained UserData"
    verify()
    for name, content in test_files.items():
        assert (target / name).read_bytes() == content, name
    # Also exercise ordinary uninstall (NSIS copies its uninstaller into Temp).
    assert run_silent(f'"{target / "uninstall.exe"}" /S') == 0
    deadline = time.monotonic() + 8
    while any((target / name).exists() for name in manifest) and time.monotonic() < deadline:
        time.sleep(0.1)
    assert not any((target / name).exists() for name in manifest)
    for name, content in test_files.items():
        assert (target / name).read_bytes() == content, name
    assert registry_snapshot() == registry_before
    print(json.dumps({"initial_install": "passed", "upgrade": "passed", "unicode_space_path": "passed",
                      "unrelated_folder_rejected": True, "locked_executable_rejected": True,
                      "locked_uninstall_rejected": True, "ordinary_uninstall_preserves_data": True,
                      "uninstall_preserves_data": True, "reinstall_preserves_data": True,
                      "registry_unchanged": True, "real_data_used": False, "qa": str(qa)}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--setup", type=Path, required=True)
    parser.add_argument("--payload", type=Path, required=True)
    parser.add_argument("--qa", type=Path, required=True)
    args = parser.parse_args()
    main(args.setup, args.payload, args.qa)
