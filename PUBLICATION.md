# Source publication notes / 2026-10-03

Intended project repository: https://github.com/t4kUM1o/moss-launcher

The initial public source is a snapshot of the modified Moss tree, not the old
local Git history. The upstream baseline is Prism Launcher 11.1.1, commit
`7d2d3c1ec6fcf8255ebacd02d9f792688575dfb4`. Original attribution, license files
and source notices are retained. See MOSS-CHANGES.md and README-UPSTREAM.md.

For this initial publication, the upstream `.github` directory is archived as
`upstream-automation`, outside GitHub's active workflow/template directories.
It is reference material only, not Moss CI, release publishing, funding or
support configuration. No workflows are configured to run on this initial push.

libnbtplusplus source is included as ordinary source files at
`libraries/libnbtplusplus`, revision
`3538933614059f0f44388a2b16f3db25ce42285b`, with its original license notices.
The old submodule declaration is preserved as
`upstream-automation/gitmodules.reference`, not an active `.gitmodules` file.
No recursive submodule download is needed for this snapshot.

No runtime user data, accounts, logs from this user's runs, EXE/DLL packages or
dependency build caches are included. Upstream log-parser test fixtures remain
under `tests/testdata/TestLogs` as required test inputs, unchanged by Moss.
The Microsoft client ID is a public application identifier, not a secret.

This source publication is not a finished binary release or Minecraft API
approval. Follow BUILD-MOSS.md for the source-folder/workspace layout. For binary
redistribution, retain matching sources and dependency notices/sources as
described in DEPENDENCY-SOURCES.txt; this repository does not bundle Qt or vcpkg
build caches.
