# Moss Windows build / 2026-10-02

This independent GPL-3.0-only fork is based on Prism Launcher 11.1.1.
The launcher uses dynamically replaceable Qt DLLs. Do not remove the source
archives, notices or users' ability to replace these libraries when redistributing.

## Prerequisites

- Windows x64; Visual Studio C++ tools, including MSVC 14.44 and a Windows SDK.
  This build used Visual Studio 2026, MSVC 14.44.35207 for the launcher and
  MSVC 14.51.36231 for vcpkg dependencies. Release redistributable CRT DLLs from
  that installation are deployed beside the application, unmodified.
- Open-source Qt 6.11.1 `msvc2022_64`: Core, Gui, Widgets, Network,
  Concurrent, Test, Svg, NetworkAuth and image format plugins.
  Supply the corresponding Qt source modules as well when redistributing.
  Moss auth requires Qt 6.8 or newer for PKCE S256.
- CMake 3.30.5 or newer and Ninja. Default script paths are under `C:\Qt\Tools`.
- Git, vcpkg and a build-only JDK 17. JDK 17 compiles the upstream helpers
  supporting old Minecraft versions; it does not constrain game Java versions.
- Python 3.11 or newer only for the source-packaging and extra verification tools.

## Workspace layout

Extract `Source.zip` into an empty workspace; it creates `moss-prism`.
The nested libnbtplusplus source is already included; no submodule download
is needed for that extracted archive.

Clone the official https://github.com/microsoft/vcpkg repository into the sibling
`work/prism-vcpkg` directory, select commit
`3aea538b2bb21a586502c67b00eb474fdd2e3098`, and bootstrap it with
`bootstrap-vcpkg.bat -disableMetrics`.
The manifest pins dependency baseline `2d6a6cf3ac9a7cc93942c3d289a2f9c661a6f4a7`.
The downloaded vcpkg tool used here is version 2026-09-26.

Place a build JDK in `work/prism-jdk17/<jdk-directory>`, or pass its root using
`-JdkRoot`. This build used Temurin 17.0.20.1+1, Windows x64.
Adjust `-QtRoot` and `-VsRoot` to match your installation. Adjust the CMake and
Ninja paths in the script if not installed in the defaults.

From PowerShell in the workspace, run:

```powershell
.\moss-prism\build-moss-windows.ps1 -Step Configure
.\moss-prism\build-moss-windows.ps1 -Step Build -Jobs 4
.\moss-prism\build-moss-windows.ps1 -Step Test
python .\moss-prism\tests\moss_fork_checks.py
.\moss-prism\build-moss-windows.ps1 -Step Install
```

Build output is `work/moss-prism-build`; the portable application is installed in
`outputs/MossPrism-Windows-11.1.1-auth-preview`. Pass `-ReleaseRoot` to install
elsewhere without modifying an older release or its user data.
For an incremental build with unchanged, already-installed dependencies, pass
`-UseInstalledDependencies` during Configure; never use that option for a fresh
build. For Japanese-only MSVC, the script fixes the generated Ninja include-prefix
bytes without changing the Windows locale.
Always re-run Configure after changing the CMake configuration.
Windows symbolic-link tests are skipped by default because the upstream tests
can repeatedly request administrator access. They are manual, opt-in tests
using `MOSSPRISM_TEST_PRIVILEGED_LINKS=1`; they were not verified in this run.

## Dependency sources and redistribution

`DependencySources.zip` contains the actual patched vcpkg dependency sources,
their exact port recipes, the vcpkg build scripts and the Qt source modules
used by this package. It includes root Qt build files and embedded third-party
code/notices. The port recipes describe how the dependency sources were built.
Extract Qt sources to `C:/Qt/6.11.1/Src` or change `QT_SOURCE` in the packager.
The packager additionally expects the local vcpkg build trees and matching
registry recipe cache from your build.

After installing into a fresh release directory, create the source archives and
hash manifest with `python .\moss-prism\pack-moss-release.py --bundle`. The
portable ZIP contains the runtime, sources and notices together. The packager
refuses a release directory containing any UserData files. Never package personal
accounts, tokens, instances or private API keys.

The public Moss Microsoft client ID is embedded with the owner's authorization;
it is not a secret and its inclusion does not imply Minecraft API approval.
CurseForge/Imgur defaults remain empty. For redistribution as a different fork,
register your own app and pass `-MicrosoftClientId` to Configure (or an empty
string to disable login). See MICROSOFT-SETUP.txt for the exact redirect setup.
The packager accepts `--release-dir` to select a fresh installed directory.
This build does not bypass game ownership or provider access restrictions.
