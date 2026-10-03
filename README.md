# Moss Launcher

An independent desktop launcher for **Minecraft: Java Edition**, based on
[Prism Launcher 11.1.1](https://github.com/PrismLauncher/PrismLauncher/tree/11.1.1).
Moss aims to make managing game instances, mods and shaders easier.

Moss is not an official Minecraft or Prism Launcher product, and is not endorsed
by or affiliated with Mojang, Microsoft or the Prism Launcher project.

**Development preview — not a finished public release.** Minecraft API approval,
successful real-account sign-in and a complete Minecraft launch have not been
verified. Registering an Azure app alone does not grant Minecraft API access.
No ready-to-use public binary release is promised by this repository.

## Current implementation

- Prism-based instance management and per-instance settings.
- Inherited Fabric/Forge loader installation and version management.
- Inherited Modrinth mod search, installation and update tools, plus modpack,
  resource-pack and shader management.
- Java discovery and manual selection, including `JAVA_HOME` and Windows `PATH`.
  A newer Java version is not guaranteed compatible with every game or mod loader.
- Separate Moss branding and portable Windows data storage, without overwriting
  existing Prism Launcher data.
- Microsoft browser sign-in using authorization code flow, PKCE S256 and OAuth
  state, with a loopback-only callback. This authentication integration still
  needs service approval and end-to-end verification.

CurseForge integration is inherited, but no CurseForge API key is bundled.
Its availability requires authorized provider credentials and applicable
permissions. Prism's provider credentials are not reused.

## Planned, not yet implemented

- A guided way to copy an existing instance and migrate it to Fabric or Forge,
  preserving the original instance and saves.
- Compatible convenience-mod recommendations and dependency guidance.
- A simplified shader setup flow with required rendering-mod guidance.
- Windows-protected credential storage and a further authentication/logging
  security review before a public binary release.

These are development goals, not features claimed to work in this preview.
Forge and Fabric mods are not generally interchangeable.

## Microsoft authentication and local data

Users sign in on Microsoft's page in their own browser; Moss does not ask users
to enter their Microsoft password into the launcher. Minecraft API access is
needed for authentication, game-entitlement checks and profile retrieval before
launching. Game ownership and provider restrictions are not bypassed.

The registered desktop redirect URI is `http://localhost/oauth/microsoft`.
The receiver listens on loopback with a temporary local port; it is not a hosted
authentication server. A browser callback does not by itself prove that Minecraft
authentication or ownership verification has completed.

The Microsoft application client ID identifies the app and is public, not a
client secret. No Microsoft client secret is bundled. Developers distributing
their own fork must register and obtain permission for their own application.
See [MICROSOFT-SETUP.txt](MICROSOFT-SETUP.txt).

**Security limitation:** account persistence still uses Prism's JSON storage;
Windows OS-protected token storage is not implemented yet. The portable
`UserData` directory can contain accounts, authentication tokens, instances,
worlds, mods and settings. Never upload it or include it in a shared package.
Do not publish passwords, access/refresh tokens, private provider API keys or
login callback URLs. This overview is not a substitute for a reviewed release
privacy policy.

## Source and building

The upstream baseline is commit
`7d2d3c1ec6fcf8255ebacd02d9f792688575dfb4` (Prism Launcher 11.1.1).
See [MOSS-CHANGES.md](MOSS-CHANGES.md) for modifications,
[BUILD-MOSS.md](BUILD-MOSS.md) for Windows build instructions and
[VERIFICATION.txt](VERIFICATION.txt) for completed checks and limitations.
Initial source snapshot and archived automation details are documented in
[PUBLICATION.md](PUBLICATION.md).
The scripts currently expect a workspace with the source folder named
`moss-prism`; follow the documented layout rather than assuming an arbitrary
checkout folder will work unchanged.

Windows builds use Qt 6.11.1; Microsoft PKCE integration requires Qt 6.8 or newer.
Building requires development tools, but running a packaged launcher does not
require Python. Minecraft and game Java are not bundled. Keep the whole runtime
folder together: an EXE alone will not include its required DLLs.

## Licenses and attribution

Launcher code is **GPL-3.0-only**. Retain [LICENSE](LICENSE),
[COPYING.md](COPYING.md), library-specific notices, attribution and corresponding
source when redistributing. Retained upstream logos and related assets have their
own CC BY-SA 4.0 attribution; they are not the Moss application mark.
Runtime dependency notices and corresponding sources are described in
[DEPENDENCY-SOURCES.txt](DEPENDENCY-SOURCES.txt).

Thanks to Prism Launcher, MultiMC and their contributors. The original project
description and acknowledgements are retained in
[README-UPSTREAM.md](README-UPSTREAM.md). Upstream support and download links
there are for Prism, not this fork. Report Moss-specific issues to the Moss
repository, not the Prism project's issue queue.

## 日本語での概要

Moss LauncherはPrism Launcherを基にした独立した開発中のランチャーです。
MODやシェーダーを入れやすくすることを目指しています。
現在はPrism由来の構成管理・MOD管理とMoss向けの変更が中心です。
構成の安全なFabric/Forge移行、おすすめMODの専用画面などはまだ未実装です。
Microsoft/Minecraftの認証は承認・実アカウント検証が完了していません。
公開されたソースと、完成して一般配布できる実行ファイルは別のものです。

変更日: 2026-10-03
