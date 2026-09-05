# VS Code開発環境

このリポジトリは、`solist_ai_project_template`のCMake／ARM GCC構成を参考に、
ML63Q2557版Plant Doctorの既存ソースとメモリ配置を維持したままVS Codeから
ビルド、書き込み、デバッグできるようにしています。

LEXIDE-Ωの`.project`、`.cproject`、`Debug/`、`Release/`は移行時の比較と
復旧のため残しています。通常の開発ではリポジトリのルートをVS Codeで開き、
ルートの`CMakeLists.txt`を使用します。

## 初回セットアップ

前提となるローカル環境は次のとおりです。

- VS Code
- PlatformIO IDE（CMakeとNinjaの配置、およびARM GCCの取得に使用）
- LEXIDE-Ω 2.2.0（付属OpenOCD/GDBをMCU-Linkデバッグに使用）
- ARM CMSIS Pack 5.9.0以降（このPCでは6.3.0を自動検出）
- ROHM ML63Q25x7_DFP 1.1.0

リポジトリのルートで次を一度実行します。ARM GNU Toolchain 14.2.1が
PlatformIOのユーザー領域へインストールされます。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-toolchain.ps1
```

VS Codeでこのフォルダを開くと、CMake Tools、C/C++、Cortex-Debug、
Serial Monitorが推奨されます。未インストールの拡張機能を追加してください。

## ビルド

`Ctrl+Shift+B`でDebugビルドを実行できます。コマンドラインでは次を使用します。

```powershell
# Debug
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Preset debug

# Release
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Preset release
```

成果物はそれぞれ`build/debug/`、`build/release/`へ生成されます。

```text
PlantDoctor.elf
PlantDoctor.hex
PlantDoctor.bin
PlantDoctor.map
compile_commands.json
```

CMakeは次の順でCMSIS Coreヘッダを探します。

1. `CMSIS_CORE_INCLUDE`で指定したパス
2. `external/CMSIS/CMSIS/Core/Include`
3. LEXIDEでインストール済みの`%LOCALAPPDATA%/Arm/Packs/ARM/CMSIS/*`

ARM GCCを別の場所へインストールした場合は、`ARM_GCC_ROOT`をツールチェーンの
ルート（直下に`bin/`があるディレクトリ）へ設定してください。

## MCU-Linkで書き込み・デバッグ

DT-EBML63Q2557を給電し、MCU-LinkのSWDIO、SWCLK、GND、ターゲット基準電圧を
接続します。基板がUSB給電されている場合、MCU-Linkのターゲット電源出力は
使用しません。

- `F5`：Debugビルド、書き込み、`main`まで実行して停止
- `Terminal > Run Task > Flash: Debug (MCU-Link)`：書き込み、検証、リセットだけ実行

デバッグ構成はLEXIDE付属のArm版OpenOCD（`openocd_arm.exe`）／GDBと
ROHM DFP 1.1.0を参照します。
別バージョンへ更新した場合は`.vscode/launch.json`のDFPパスを更新してください。
`scripts/flash.ps1`はインストール済みDFPのうち最も新しい版を自動選択します。

## テンプレートとの差分

参照テンプレートはML63Q2537向けですが、このプロジェクトは実機に合わせて
`ML63Q2557`を定義します。テンプレートのサンプル`main.c`やドライバで既存実装を
置き換えず、Plant Doctorの38翻訳単位をそのままCMakeターゲットにしています。

Solist-AIのプリビルドライブラリは、現在の`PlantAi`がまだスタブでAPIを使用して
いないため、この移行ではコピーしていません。利用を開始するときは、参照
テンプレートのライセンスとROHMファイルの再配布条件を確認してから追加します。

また、`CommonFiles/`など既存のROHM提供ソースにも利用条件があります。
GitHubへ移す場合は非公開リポジトリを基本とし、公開前に各ファイルの通知と
再配布権限を確認してください。
