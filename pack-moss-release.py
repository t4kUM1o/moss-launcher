"""Moss fork release packager, 2026-10-02. SPDX-License-Identifier: GPL-3.0-only."""
import hashlib
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

SOURCE = Path(__file__).resolve().parent
WORK = SOURCE.parent / "work"
RELEASE = SOURCE.parent / "outputs/MossPrism-Windows-11.1.1-auth-preview"
QT_SOURCE = Path("C:/Qt/6.11.1/Src")
DEPENDENCIES = ("bzip2", "cmark", "ecm", "libarchive", "liblzma", "libqrencode", "lz4", "lzo", "pkgconf", "tomlplusplus", "zlib", "zstd")
BUILD_PORTS = DEPENDENCIES + ("vcpkg-cmake", "vcpkg-cmake-config", "vcpkg-cmake-get-vars", "vcpkg-tool-meson")


def git_files(root):
    result = subprocess.check_output(["git", "-c", f"safe.directory={root.resolve().as_posix()}", "-c", "core.quotepath=false",
                                      "ls-files", "-z", "--cached", "--others", "--exclude-standard"], cwd=root)
    return [root / item.decode("utf-8") for item in result.split(b"\0") if item]


def add_tree(archive, root, prefix):
    if not root.is_dir():
        raise RuntimeError(f"Missing corresponding source directory: {root}")
    for item in sorted(root.rglob("*")):
        if item.is_file() and not any(part in {".git", "__pycache__"} for part in item.relative_to(root).parts):
            archive.write(item, str(Path(prefix) / item.relative_to(root)))


def write_manifest():
    manifest = {}
    for item in sorted(RELEASE.rglob("*")):
        if item.is_file() and item.name != "SHA256.json" and "UserData" not in item.relative_to(RELEASE).parts:
            with item.open("rb") as stream:
                manifest[str(item.relative_to(RELEASE))] = hashlib.file_digest(stream, "sha256").hexdigest()
    (RELEASE / "SHA256.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(json.dumps({"exe": str(RELEASE / "mossprism.exe"), "sha256": manifest["mossprism.exe"], "files": len(manifest)}))


def pack(refresh_source=False):
    executable = RELEASE / "mossprism.exe"
    if not executable.is_file():
        raise SystemExit("Install the built EXE before packaging.")
    if any(item.is_file() for item in (RELEASE / "UserData").rglob("*")):
        raise SystemExit("Do not package runtime user data. Install into a fresh release folder.")
    for filename in ("LICENSE", "COPYING.md", "MOSS-CHANGES.md", "README-MOSS.txt", "DEPENDENCY-SOURCES.txt", "BUILD-MOSS.md", "MSVC-RUNTIME-NOTICE.txt", "VERIFICATION.txt", "MICROSOFT-SETUP.txt"):
        shutil.copy2(SOURCE / filename, RELEASE / filename)
    licenses = RELEASE / "licenses"
    licenses.mkdir(exist_ok=True)
    for name in DEPENDENCIES:
        copyright_file = WORK / f"moss-prism-build/vcpkg_installed/x64-windows/share/{name}/copyright"
        if copyright_file.is_file():
            shutil.copy2(copyright_file, licenses / f"{name}.txt")
    for module in ("qtbase", "qtsvg", "qtnetworkauth", "qtimageformats"):
        module_licenses = QT_SOURCE / module / "LICENSES"
        if module_licenses.is_dir():
            shutil.copytree(module_licenses, licenses / module, dirs_exist_ok=True)
    with zipfile.ZipFile(RELEASE / "Source.zip", "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for item in git_files(SOURCE):
            if item.is_file() and "__pycache__" not in item.parts:
                archive.write(item, str(Path("moss-prism") / item.relative_to(SOURCE)))
        library = SOURCE / "libraries/libnbtplusplus"
        for item in git_files(library):
            if item.is_file():
                archive.write(item, str(Path("moss-prism/libraries/libnbtplusplus") / item.relative_to(library)))
    if refresh_source:
        dependency_archive = RELEASE / "DependencySources.zip"
        with zipfile.ZipFile(dependency_archive) as archive:
            if archive.testzip() is not None:
                raise RuntimeError("Dependency source archive checksum verification failed")
        write_manifest()
        return
    # Supply the actual patched dependency sources used by vcpkg, not only links.
    with zipfile.ZipFile(RELEASE / "DependencySources.zip", "w", zipfile.ZIP_DEFLATED, compresslevel=3) as archive:
        vcpkg = WORK / "prism-vcpkg"
        for directory in ("scripts", "triplets"):
            add_tree(archive, vcpkg / directory, f"vcpkg-toolchain/{directory}")
        for filename in ("LICENSE.txt", "NOTICE.txt", "bootstrap-vcpkg.bat", "bootstrap-vcpkg.sh"):
            archive.write(vcpkg / filename, f"vcpkg-toolchain/{filename}")
        toolchain_commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=vcpkg, text=True).strip()
        archive.writestr("vcpkg-toolchain/COMMIT.txt", toolchain_commit + "\n")
        # Include only the exact public recipes used in this build, identified
        # by their recorded ABI hash, not unrelated cached ports.
        port_cache = Path(os.environ["LOCALAPPDATA"]) / "vcpkg/registries/git-trees"
        port_directories = (list(port_cache.iterdir()) + [vcpkg / "ports" / name for name in BUILD_PORTS]
                            + [SOURCE / "cmake/vcpkg-ports" / name for name in BUILD_PORTS])
        for name in BUILD_PORTS:
            installed = WORK / f"moss-prism-build/vcpkg_installed/x64-windows/share/{name}"
            abi_lines = (installed / "vcpkg_abi_info.txt").read_text().splitlines()
            port_hash = next(line.split()[1] for line in abi_lines if line.startswith("portfile.cmake "))
            json_hash = next(line.split()[1] for line in abi_lines if line.startswith("vcpkg.json "))
            matches = [directory for directory in port_directories
                       if (directory / "portfile.cmake").is_file()
                       and (directory / "vcpkg.json").is_file()
                       and hashlib.sha256((directory / "portfile.cmake").read_bytes()).hexdigest() == port_hash
                       and hashlib.sha256((directory / "vcpkg.json").read_bytes()).hexdigest() == json_hash]
            if not matches:
                raise RuntimeError(f"Missing exact build recipe for {name}")
            add_tree(archive, matches[0], f"vcpkg-build-recipes/{name}")
        for name in DEPENDENCIES:
            candidates = list((WORK / f"prism-vcpkg/buildtrees/{name}/src").glob("*.clean"))
            if len(candidates) != 1:
                raise RuntimeError(f"Expected exactly one built source tree for {name}, found {len(candidates)}")
            add_tree(archive, candidates[0], f"vcpkg-sources/{name}")
        for module in ("qtbase", "qtsvg", "qtnetworkauth", "qtimageformats"):
            add_tree(archive, QT_SOURCE / module, f"qt-6.11.1/{module}")
        for root in ("cmake", "LICENSES"):
            add_tree(archive, QT_SOURCE / root, f"qt-6.11.1/{root}")
        for item in QT_SOURCE.iterdir():
            if item.is_file() and item.name != ".git":
                archive.write(item, f"qt-6.11.1/{item.name}")
        archive.write(SOURCE / "DEPENDENCY-SOURCES.txt", "DEPENDENCY-SOURCES.txt")
    write_manifest()


def bundle():
    destination = RELEASE.parent / f"{RELEASE.name}.zip"
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=3) as archive:
        archive.writestr(f"{RELEASE.name}/UserData/", "")
        for item in sorted(RELEASE.rglob("*")):
            if item.is_file() and "UserData" not in item.relative_to(RELEASE).parts:
                archive.write(item, str(Path(RELEASE.name) / item.relative_to(RELEASE)),
                              compress_type=zipfile.ZIP_STORED if item.suffix == ".zip" else zipfile.ZIP_DEFLATED)
    with destination.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    print(json.dumps({"bundle": str(destination), "sha256": digest, "bytes": destination.stat().st_size}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--release-dir", type=Path, help="Fresh installed release folder; must not contain user data.")
    parser.add_argument("--refresh-source", action="store_true", help="Refresh launcher source/notices only; validate and retain the existing dependency source ZIP.")
    parser.add_argument("--bundle", action="store_true", help="Also create a portable ZIP containing the runtime, sources and notices; never include user data.")
    arguments = parser.parse_args()
    if arguments.release_dir:
        RELEASE = arguments.release_dir.resolve()
    pack(arguments.refresh_source)
    if arguments.bundle:
        bundle()
