# ベータ版インストーラーの作成

現在のパッケージ版は `0.1.0-beta.1`、ベースのPrismは11.1.1です。
ランチャー本体のゲーム機能は変更せず、検証済みのWindows x64バイナリを使用します。

1. `build-moss-windows.ps1` でビルド・テスト・別フォルダへのインストールを実施します。
2. `pack-moss-release.py` で対応ソース・依存ソース・SHA256.jsonをまとめます。
3. [公式配布](https://nsis.sourceforge.io/Download)のNSIS 3.13 ZIPを作業フォルダに展開します。
4. 作業フォルダのPowerShellから次のように実行します。

```powershell
$env:PATH = 'C:\Program Files\Git\cmd;' + $env:PATH
python .\moss-prism\installer\build-beta-installer.py `
  --runtime .\outputs\MossPrism-Windows-11.1.1-mod-dependencies `
  --nsis .\work\moss-installer-tools\extracted\nsis-3.13 `
  --output .\outputs\MossLauncher-0.1.0-beta.1-installer
```

出力先は新規フォルダである必要があります。入力のSHA256.jsonに載ったファイルだけを
検証して使用し、UserDataを列挙・コピーしません。portable.txtも含めず、インストール時に
空のUserDataフォルダを作ります。ランチャーは、そのフォルダを保存先として認識します。
対応ソースは最新のソースツリーから作り直し、インストーラーのソースも含めます。

NSISにはファイル単位の明示的な一覧を渡します。アンインストールもその一覧だけを削除し、
再帰的な削除や強制終了、管理者権限、ネット通信、外部ランタイムのインストールは使用しません。
日本語の案内、任意のデスクトップショートカット、ユーザー単位のアプリ一覧登録を行います。
旧Moss・Prismの関連付けは変更しません。未知のファイルがあるフォルダやリンク先は拒否します。

`/S /NoIntegration /D=<テスト専用フォルダ>` は、登録とショートカットを作らない
検証用のサイレント実行です。既存フォルダではなく、作業環境内の新規テストフォルダだけで
使用してください。アンインストール後もUserDataと所有確認用INIは残します。
サイレントモードの動作確認と、実際の対話画面・ゲーム起動の検証は別です。

コード署名とGitHub Releaseへの公開はこのスクリプトでは行いません。
配布時は、作成されたインストーラーとSHA256、ベータ版の注意事項を一緒に渡してください。
