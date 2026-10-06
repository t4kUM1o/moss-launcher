; Moss beta installer, 2026-10-06. SPDX-License-Identifier: GPL-3.0-only
; Payload instructions are generated from the audited release manifest, not File /r.
Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "WinVer.nsh"
!include "x64.nsh"

!define APP_ID "MossLauncherBeta"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_ID}"
!define INSTALL_MARKER "moss-beta-install.ini"
Name "Moss Launcher ベータ版 ${BETA_VERSION}"
OutFile "${SETUP_OUT}"
InstallDir "$LOCALAPPDATA\Programs\Moss Launcher Beta"
RequestExecutionLevel user
SetCompressor /SOLID zlib
CRCCheck on
AllowRootDirInstall false
AllowSkipFiles off
ManifestDPIAware true
VIProductVersion "0.1.0.1"
VIAddVersionKey /LANG=1041 "ProductName" "Moss Launcher Beta"
VIAddVersionKey /LANG=1041 "ProductVersion" "${BETA_VERSION}"
VIAddVersionKey /LANG=1041 "FileVersion" "0.1.0.1"
VIAddVersionKey /LANG=1041 "FileDescription" "Moss Launcher ベータ版セットアップ"
VIAddVersionKey /LANG=1041 "LegalCopyright" "Moss fork; Prism Launcher Contributors"
!define MUI_ICON "${SOURCE_ROOT}\program_info\mossprism.ico"
!define MUI_UNICON "${SOURCE_ROOT}\program_info\mossprism.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "Moss Launcher ベータ版のセットアップ"
!define MUI_WELCOMEPAGE_TEXT "これは ${BETA_VERSION} の試験版です。Microsoft/Minecraft API承認・実ゲーム起動は未確認です。$\r$\n$\r$\nこのWindowsユーザー専用にインストールします。管理者権限は要求しません。既存の起動構成は自動移行しません。$\r$\n$\r$\nアプリとゲームを閉じてから進んでください。"
!insertmacro MUI_PAGE_WELCOME
!define MUI_PAGE_HEADER_TEXT "ベータ版の注意事項"
!define MUI_PAGE_HEADER_SUBTEXT "制限事項・保存先・ライセンスを確認してください。"
!define MUI_LICENSEPAGE_BUTTON "確認して次へ"
!define MUI_LICENSEPAGE_TEXT_TOP "このベータ版の注意事項です。ライセンス原文もインストール先へ同梱します。"
!insertmacro MUI_PAGE_LICENSE "${PAYLOAD_ROOT}\README-BETA.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "セットアップが完了しました。$\r$\n$\r$\n初回は日本語とJavaを選択してください。保存データはインストール先のUserDataに作成されます。"
!define MUI_FINISHPAGE_RUN "$INSTDIR\mossprism.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Moss Launcherを起動する"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README-BETA.txt"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "ベータ版の注意事項を読む"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!insertmacro MUI_PAGE_FINISH
!define MUI_UNCONFIRMPAGE_TEXT_TOP "アプリのファイルと登録を削除します。UserData内の起動構成・ワールド・MOD・アカウント情報は残します。"
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "Japanese"
Var NoIntegration

!macro CHECK_REPARSE PATH
  System::Call 'kernel32::GetFileAttributesW(w "${PATH}") i .r9'
  ${If} $9 != -1
    IntOp $9 $9 & 0x400
    ${If} $9 != 0
      MessageBox MB_OK|MB_ICONSTOP "リンク先のフォルダやファイルにはインストール・削除できません。通常のフォルダを選んでください。" /SD IDOK
      SetErrorLevel 103
      Quit
    ${EndIf}
  ${EndIf}
!macroend

!macro CHECK_PARENTS
  ; NSIS GetFullPathName clears the output for a new/nonexistent directory.
  ; Win32 canonicalization supports the fresh install folder without creating it.
  System::Call 'kernel32::GetFullPathNameW(w "$INSTDIR", i ${NSIS_MAX_STRLEN}, w .r6, p 0) i .r5'
  ${If} $5 == 0
  ${OrIf} $5 >= ${NSIS_MAX_STRLEN}
    SetErrorLevel 102
    Quit
  ${EndIf}
  StrCpy $INSTDIR "$6"
  ${GetRoot} "$INSTDIR" $7
  ${If} $INSTDIR == $7
    MessageBox MB_OK|MB_ICONSTOP "ドライブのルートは使用できません。専用のフォルダを選んでください。" /SD IDOK
    SetErrorLevel 102
    Quit
  ${EndIf}
  StrCpy $7 "$INSTDIR"
  ${Do}
    !insertmacro CHECK_REPARSE "$7"
    ${GetParent} "$7" $8
    ${If} $8 == ""
      ${ExitDo}
    ${EndIf}
    ${If} $8 == $7
      ${ExitDo}
    ${EndIf}
    StrCpy $7 "$8"
  ${Loop}
!macroend

!macro CHECK_EXE_LOCK
  ${If} ${FileExists} "$INSTDIR\mossprism.exe"
    System::Call 'kernel32::CreateFileW(w "$INSTDIR\mossprism.exe", i 0x40000000, i 0, p 0, i 3, i 0, p 0) p .r0'
    ${If} $0 == -1
      MessageBox MB_OK|MB_ICONSTOP "Moss Launcherが使用中か、書き込みできません。アプリとゲームを終了してから、もう一度実行してください。強制終了は行いません。" /SD IDOK
      SetErrorLevel 105
      Quit
    ${EndIf}
    System::Call 'kernel32::CloseHandle(p r0)'
  ${EndIf}
!macroend

Function .onInit
  SetShellVarContext current
  SetRegView 64
  ${IfNot} ${IsNativeAMD64}
    MessageBox MB_OK|MB_ICONSTOP "このベータ版はWindows x64専用です。" /SD IDOK
    SetErrorLevel 100
    Quit
  ${EndIf}
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_OK|MB_ICONSTOP "Windows 10または11が必要です。" /SD IDOK
    SetErrorLevel 101
    Quit
  ${EndIf}
  StrCpy $NoIntegration "0"
  ${GetParameters} $0
  ClearErrors
  ${GetOptions} $0 "/NoIntegration" $1
  ${IfNot} ${Errors}
    StrCpy $NoIntegration "1"
  ${Else}
    ReadRegStr $0 HKCU "Software\${APP_ID}" "InstallDir"
    ${If} $0 != ""
      StrCpy $INSTDIR "$0"
    ${EndIf}
  ${EndIf}
FunctionEnd

Function CheckTarget
  !insertmacro CHECK_PARENTS
  !insertmacro CHECK_REPARSE "$INSTDIR\${INSTALL_MARKER}"
  !insertmacro CHECK_REPARSE "$INSTDIR\uninstall.exe"
  ReadINIStr $0 "$INSTDIR\${INSTALL_MARKER}" "Moss" "AppID"
  ${If} $0 != "${APP_ID}"
    FindFirst $1 $2 "$INSTDIR\*"
    ${DoWhile} $2 != ""
      ${If} $2 != "."
      ${AndIf} $2 != ".."
        FindClose $1
        MessageBox MB_OK|MB_ICONSTOP "このフォルダには既存のファイルがあります。Mossベータ版専用の空のフォルダを選んでください。" /SD IDOK
        SetErrorLevel 104
        Quit
      ${EndIf}
      FindNext $1 $2
    ${Loop}
    FindClose $1
  ${EndIf}
  !insertmacro CHECK_EXE_LOCK
  !insertmacro CHECK_REPARSE "$INSTDIR\UserData"
  !include "${GENERATED_ROOT}\check-paths.nsh"
FunctionEnd

Section "Moss Launcher ベータ版（必須）" Application
  SectionIn RO
  Call CheckTarget
  SetOverwrite on
  !include "${GENERATED_ROOT}\install-files.nsh"
  CreateDirectory "$INSTDIR\UserData"
  WriteINIStr "$INSTDIR\${INSTALL_MARKER}" "Moss" "AppID" "${APP_ID}"
  WriteINIStr "$INSTDIR\${INSTALL_MARKER}" "Moss" "Version" "${BETA_VERSION}"
  WriteINIStr "$INSTDIR\${INSTALL_MARKER}" "Moss" "NoIntegration" "$NoIntegration"
  WriteUninstaller "$INSTDIR\uninstall.exe"
  ${If} $NoIntegration == "0"
    WriteRegStr HKCU "Software\${APP_ID}" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayName" "Moss Launcher ベータ版"
    WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayVersion" "${BETA_VERSION}"
    WriteRegStr HKCU "${UNINSTALL_KEY}" "Publisher" "Moss Launcher"
    WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\mossprism.exe"
    WriteRegStr HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "${UNINSTALL_KEY}" "URLInfoAbout" "https://github.com/t4kUM1o/moss-launcher"
    WriteRegStr HKCU "${UNINSTALL_KEY}" "UninstallString" '$\"$INSTDIR\uninstall.exe$\"'
    WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair" 1
    WriteRegDWORD HKCU "${UNINSTALL_KEY}" "EstimatedSize" ${PAYLOAD_KIB}
    CreateDirectory "$SMPROGRAMS\Moss Launcher Beta"
    CreateShortcut "$SMPROGRAMS\Moss Launcher Beta\Moss Launcher Beta.lnk" "$INSTDIR\mossprism.exe"
    CreateShortcut "$SMPROGRAMS\Moss Launcher Beta\注意事項.lnk" "$INSTDIR\README-BETA.txt"
    CreateShortcut "$SMPROGRAMS\Moss Launcher Beta\アンインストール.lnk" "$INSTDIR\uninstall.exe"
  ${EndIf}
SectionEnd
Section /o "デスクトップにショートカットを作成" DesktopShortcut
  ${If} $NoIntegration == "0"
    CreateShortcut "$DESKTOP\Moss Launcher Beta.lnk" "$INSTDIR\mossprism.exe"
  ${EndIf}
SectionEnd

Function un.onInit
  SetShellVarContext current
  SetRegView 64
  !insertmacro CHECK_PARENTS
  !insertmacro CHECK_REPARSE "$INSTDIR\${INSTALL_MARKER}"
  !insertmacro CHECK_REPARSE "$INSTDIR\uninstall.exe"
  ReadINIStr $0 "$INSTDIR\${INSTALL_MARKER}" "Moss" "AppID"
  ${If} $0 != "${APP_ID}"
    MessageBox MB_OK|MB_ICONSTOP "Mossベータ版のインストール先を確認できません。何も削除しません。" /SD IDOK
    SetErrorLevel 106
    Quit
  ${EndIf}
  ReadINIStr $NoIntegration "$INSTDIR\${INSTALL_MARKER}" "Moss" "NoIntegration"
  !insertmacro CHECK_EXE_LOCK
  !include "${GENERATED_ROOT}\check-paths.nsh"
FunctionEnd

Section "Uninstall"
  ; Delete only the exact payload paths. Never recursively remove UserData/app root.
  !include "${GENERATED_ROOT}\uninstall-files.nsh"
  Delete "$INSTDIR\uninstall.exe"
  ${If} $NoIntegration == "0"
    ReadRegStr $0 HKCU "Software\${APP_ID}" "InstallDir"
    ${If} $0 == $INSTDIR
      DeleteRegKey HKCU "${UNINSTALL_KEY}"
      DeleteRegKey HKCU "Software\${APP_ID}"
      Delete "$SMPROGRAMS\Moss Launcher Beta\Moss Launcher Beta.lnk"
      Delete "$SMPROGRAMS\Moss Launcher Beta\注意事項.lnk"
      Delete "$SMPROGRAMS\Moss Launcher Beta\アンインストール.lnk"
      RMDir "$SMPROGRAMS\Moss Launcher Beta"
      Delete "$DESKTOP\Moss Launcher Beta.lnk"
    ${EndIf}
  ${EndIf}
  ; Retain the ownership marker so reinstall into retained UserData is safe.
SectionEnd
