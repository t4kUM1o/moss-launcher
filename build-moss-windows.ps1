# SPDX-License-Identifier: GPL-3.0-only
# Moss fork build workflow, added 2026-10-02. No global environment changes.
param(
    [ValidateSet('Configure', 'Build', 'Test', 'Install', 'All')]
    [string]$Step = 'All',
    [string]$QtRoot = 'C:\Qt\6.11.1\msvc2022_64',
    [string]$VsRoot = 'C:\Program Files\Microsoft Visual Studio\18\Community',
    [string]$JdkRoot = '',
    [string]$RuntimeRoot = '',
    [string]$MicrosoftClientId = '191f8f9d-bc7c-4f13-8989-ad109a11bf9a',
    [string]$ReleaseRoot = '',
    [switch]$UseInstalledDependencies,
    [int]$Jobs = 4
)
$ErrorActionPreference = 'Stop'
$mossSource = $PSScriptRoot
$mossWorkspace = Split-Path $mossSource -Parent
$mossWork = Join-Path $mossWorkspace 'work'
$mossBuild = Join-Path $mossWork 'moss-prism-build'
if (!$ReleaseRoot) { $ReleaseRoot = Join-Path $mossWorkspace 'outputs\MossPrism-Windows-11.1.1-auth-preview' }
$mossRelease = [IO.Path]::GetFullPath($ReleaseRoot)
$mossVcpkg = Join-Path $mossWork 'prism-vcpkg'
if (!$RuntimeRoot) { $RuntimeRoot = Join-Path $VsRoot 'VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT' }
if (!$JdkRoot) {
    $mossJdkFolders = @(Get-ChildItem (Join-Path $mossWork 'prism-jdk17') -Directory)
    if ($mossJdkFolders.Count -ne 1) { throw 'Pass -JdkRoot for a build-only JDK 17.' }
    $JdkRoot = $mossJdkFolders[0].FullName
}
$mossCmake = 'C:\Qt\Tools\CMake_64\bin\cmake.exe'
$mossCtest = 'C:\Qt\Tools\CMake_64\bin\ctest.exe'
$env:VSLANG = '1033'
Import-Module (Join-Path $VsRoot 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $VsRoot -SkipAutomaticLocation -DevCmdArguments '-arch=amd64 -host_arch=amd64 -vcvars_ver=14.44'
$env:VSLANG = '1033'
$mossClDirectory = Split-Path (Get-Command cl.exe).Source -Parent
$mossJapaneseOnly = (Test-Path (Join-Path $mossClDirectory '1041')) -and !(Test-Path (Join-Path $mossClDirectory '1033'))
if ($mossJapaneseOnly) { $env:VSLANG = '1041' }
$env:PATH = "$JdkRoot\bin;$QtRoot\bin;C:\Qt\Tools\Ninja;C:\Program Files\Git\cmd;C:\Program Files\Git\usr\bin;$env:PATH"
$env:JAVA_HOME = $JdkRoot
$env:VCPKG_ROOT = $mossVcpkg
$env:VCPKG_DOWNLOADS = Join-Path $mossWork 'prism-downloads'
$env:VCPKG_DEFAULT_BINARY_CACHE = Join-Path $mossWork 'prism-binary-cache'
$env:VCPKG_REGISTRIES_CACHE = Join-Path $mossWork 'prism-registry-cache'
$env:VCPKG_VISUAL_STUDIO_PATH = $VsRoot
# Use Git's verified OpenSSL backend without changing the user's Git settings.
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'http.sslBackend'
$env:GIT_CONFIG_VALUE_0 = 'openssl'
New-Item -ItemType Directory -Force -Path $env:VCPKG_DOWNLOADS, $env:VCPKG_DEFAULT_BINARY_CACHE, $env:VCPKG_REGISTRIES_CACHE | Out-Null
function Assert-MossExit([string]$Operation) {
    if ($LASTEXITCODE -ne 0) { throw "$Operation failed: $LASTEXITCODE" }
}
function Repair-MossNinjaPrefix {
    # Normalize Ninja's Japanese include prefix to UTF-8. Correct only that line;
    # retain UTF-8 paths and the rest of Ninja's generated file unchanged.
    $mossRules = Join-Path $mossBuild 'CMakeFiles\rules.ninja'
    if ($mossJapaneseOnly -and (Test-Path $mossRules)) {
        $mossByteEncoding = [Text.Encoding]::GetEncoding(28591)
        # ASCII escapes also work when Windows PowerShell 5 reads UTF-8 source
        # without a BOM using the system ANSI encoding.
        $mossUnicodePrefix = [regex]::Unescape('\u30e1\u30e2: \u30a4\u30f3\u30af\u30eb\u30fc\u30c9 \u30d5\u30a1\u30a4\u30eb: ')
        $mossPrefixBytes = [Text.Encoding]::UTF8.GetBytes($mossUnicodePrefix)
        $mossPrefixText = $mossByteEncoding.GetString($mossPrefixBytes)
        $mossRuleText = $mossByteEncoding.GetString([IO.File]::ReadAllBytes($mossRules))
        $mossRuleText = [regex]::Replace($mossRuleText, '(?m)^msvc_deps_prefix = [^\r\n]*', "msvc_deps_prefix = $mossPrefixText")
        [IO.File]::WriteAllBytes($mossRules, $mossByteEncoding.GetBytes($mossRuleText))
    }
}
if ($Step -in 'Configure', 'All') {
    & $mossCmake -S $mossSource -B $mossBuild -G 'Ninja Multi-Config' `
        '-DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe' `
        "-DCMAKE_PREFIX_PATH=$QtRoot" "-DCMAKE_TOOLCHAIN_FILE=$mossVcpkg/scripts/buildsystems/vcpkg.cmake" `
        '-DVCPKG_TARGET_TRIPLET=x64-windows' '-DVCPKG_HOST_TRIPLET=x64-windows' `
        "-DVCPKG_MANIFEST_INSTALL=$(!$UseInstalledDependencies)" `
        "-DLibArchive_LIBRARY=$mossBuild/vcpkg_installed/x64-windows/lib/archive.lib" `
        '-DLauncher_BUILD_PLATFORM=moss-local-windows' '-DLauncher_BUILD_ARTIFACT=' `
        "-DLauncher_MSA_CLIENT_ID=$MicrosoftClientId" '-DLauncher_LOGIN_CALLBACK_URL=' `
        '-DLauncher_CURSEFORGE_API_KEY=' '-DLauncher_IMGUR_CLIENT_ID=' `
        '-DLauncher_UPDATER_GITHUB_REPO=' '-DLauncher_USE_PCH=ON' '-DBUILD_TESTING=ON' `
        '-DLauncher_ENABLE_JAVA_DOWNLOADER=ON' '-DENABLE_LTO=OFF' "-DCMAKE_INSTALL_PREFIX=$mossRelease" `
        "-DJava_JAVA_EXECUTABLE=$JdkRoot/bin/java.exe" "-DJava_JAVAC_EXECUTABLE=$JdkRoot/bin/javac.exe" `
        "-DJava_JAR_EXECUTABLE=$JdkRoot/bin/jar.exe"
    Assert-MossExit 'Configure'
    Repair-MossNinjaPrefix
}
if ($Step -in 'Build', 'All') {
    Repair-MossNinjaPrefix
    & $mossCmake --build $mossBuild --config Release --parallel $Jobs
    Assert-MossExit 'Build'
}
if ($Step -in 'Test', 'All') {
    Copy-Item -Path (Join-Path $RuntimeRoot '*.dll') -Destination (Join-Path $mossBuild 'Release')
    # Elevated helpers do not inherit PATH. Keep their Qt DLLs beside the
    # executable, even though elevation-dependent tests are opt-in by default.
    foreach ($mossQtDll in 'Core','Gui','Widgets','Network','NetworkAuth','Xml','OpenGL','Svg','Test','Concurrent') {
        Copy-Item -LiteralPath (Join-Path $QtRoot "bin\Qt6$mossQtDll.dll") -Destination (Join-Path $mossBuild 'Release')
    }
    # The build JDK can ship older CRT DLLs; do not put those in front of Qt
    # while running native tests. Keep the build-only Java out of test DLL search.
    $mossTestPath = ($env:PATH -split ';' | Where-Object { $_.TrimEnd('\') -ne "$JdkRoot\bin" }) -join ';'
    $env:PATH = "$QtRoot\bin;$mossBuild\vcpkg_installed\x64-windows\bin;$mossTestPath"
    $env:QT_QPA_PLATFORM = 'offscreen'
    & $mossCtest --test-dir $mossBuild -C Release --output-on-failure --parallel $Jobs --timeout 60
    Assert-MossExit 'Tests'
    Remove-Item Env:QT_QPA_PLATFORM
}
if ($Step -in 'Install', 'All') {
    & $mossCmake --install $mossBuild --config Release
    Assert-MossExit 'Install'
    & $mossCmake --install $mossBuild --config Release --component portable
    Assert-MossExit 'Portable marker'
    New-Item -ItemType Directory -Force -Path (Join-Path $mossRelease 'UserData') | Out-Null
    # Unmodified release redistributables only; never debug_nonredist binaries.
    Copy-Item -Path (Join-Path $RuntimeRoot '*.dll') -Destination $mossRelease
    foreach ($mossRequiredDll in 'Qt6Core.dll','Qt6Network.dll','vcruntime140.dll','msvcp140_1.dll') {
        if (!(Test-Path (Join-Path $mossRelease $mossRequiredDll))) { throw "Incomplete runtime bundle: $mossRequiredDll" }
    }
    Copy-Item -LiteralPath (Join-Path $mossSource 'LICENSE'), (Join-Path $mossSource 'COPYING.md'), (Join-Path $mossSource 'MOSS-CHANGES.md') -Destination $mossRelease
}
