# Moss Launcher (Prism-based)

Independent modified build based on Prism Launcher **11.1.1**, upstream commit
`7d2d3c1ec6fcf8255ebacd02d9f792688575dfb4`.
Changes made on **2026-10-02 / 2026-10-03**. This fork is not Prism Launcher and is not
endorsed by or affiliated with the Prism Launcher project.

## Preserved implementation

Prism's Qt screens and instance management, loader installers, MOD/modpack
management, Java discovery/testing/download, settings, logs, worlds, resource
packs, shaders and account implementation are retained. This is not a Tk UI
imitation. UI layout is upstream; the product name and application mark differ.
Authentication and provider authorization are not bypassed.

## Fork changes

- Moss-first project README, retained upstream acknowledgements, source-publication
  notes and additional runtime/credential ignore rules. The first public source
  snapshot archives upstream GitHub automation outside active `.github` paths
  and includes the pinned libnbtplusplus source as ordinary files.
- Separate application name `MossPrism`, app ID `local.moss.MossPrism`, binary
  `mossprism.exe`, config `mossprism.cfg`, environment prefix `MOSSPRISM`.
- Original Moss four-tile SVG/ICO mark. Upstream copyright notices are retained.
- Explicit fork disclosure in the About screen and this documentation.
- Moss's own public Microsoft client ID was supplied by the app owner and is
  embedded in the auth preview. Minecraft API approval and real-account login
  are NOT confirmed. CurseForge and Imgur credentials remain empty.
  Runtime service overrides remain available. No client secret is shipped.
- Microsoft auth now consistently uses a loopback-only receiver with the
  registered URI `http://localhost/oauth/microsoft` and a dynamic local port,
  PKCE S256, and state. The browser response is displayed locally, without
  forwarding to Prism's successful-login page. Callback listeners are closed
  after OAuth success/failure and are not kept open for token refresh.
  Removed the upstream raw OAuth error-response-body logger.
  Account JSON/token persistence remains upstream and is not OS-encrypted yet.
- Official Prism update channel disabled so it cannot replace this fork.
- Official Prism bug-tracker default disabled; fork issues must not be sent
  to the upstream project's issue queue automatically.
- Reproducible local Windows build script and MSVC/Ninja diagnostic-language
  alignment to preserve correct header dependency tracking on Japanese Windows.
  ASCII Unicode escapes keep the diagnostic prefix correct under Windows
  PowerShell 5 even when it reads a BOM-less UTF-8 script as ANSI.
- Java discovery additionally includes process JAVA_HOME, quoted Windows PATH
  entries, and skips empty entries. This does not modify system environment variables.
- Automatic tests skip Windows symbolic-link cases that invoke the elevated
  helper. They are opt-in only, preventing repeated UAC prompts during testing.
  Qt and unmodified release CRT DLLs are deployed beside the helper, not merely
  supplied through PATH, because elevated processes do not inherit that PATH.
- Java compatibility follows the upstream compatible-major list and existing
  `Skip Java compatibility checks` option; a newer Java is not claimed to be
  universally compatible with every old loader or MOD.

## Data safety

The Windows package is portable. A `UserData` directory next to the EXE holds
configuration and instances. The old Python Moss and installed Prism data are
not imported, overwritten or moved. To test another Java manually, turn off
Java autodetection for that instance, select Java and enable the existing
compatibility-check override when needed.

## Licensing and source

Launcher code is GPL-3.0-only. `LICENSE` and `COPYING.md` are retained, as are
library-specific notices. Unused upstream logos/assets retained in the source
are CC BY-SA 4.0 as documented in upstream README; they are not the Moss mark.
Credits: https://github.com/PrismLauncher/PrismLauncher
Asset license: https://creativecommons.org/licenses/by-sa/4.0/

Redistribution must provide Corresponding Source, including our modifications,
submodule sources and build scripts. The release packager includes this source
without Git history or private API credentials. Qt and bundled dependency
notices/source information must also accompany the Windows runtime package.

No code in the old `minecraft-launcher` directory is replaced by this fork.
