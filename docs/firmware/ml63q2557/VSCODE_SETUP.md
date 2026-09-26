# VS Code開発環境

このリポジトリは、`solist_ai_project_template`のCMake／ARM GCC構成を参考に、
ML63Q2557版Plant Doctorの既存ソースとメモリ配置を維持したまま、Windows／macOSの
どちらからでもVS Codeでビルド、書き込み、デバッグできるようにしています。

LEXIDE-Ωの`.project`、`.cproject`、`Debug/`、`Release/`は移行時の比較と
復旧のため残しています。通常の開発ではリポジトリのルートをVS Codeで開き、
`firmware/ml63q2557/`の`CMakeLists.txt`を使用します。

## 開発環境の構成

共通して使用するものは次のとおりです。

- VS Code
- CMake Tools
- C/C++
- Cortex-Debug
- PlatformIO IDEまたはPlatformIO Core
- ROHM `ML63Q25x7_DFP 1.1.0`
- MCU-Link（CMSIS-DAP / SWD）

OS別のデバッグバックエンドは次のとおりです。

| OS | Build | Flash / Debug |
| --- | --- | --- |
| Windows | PowerShell + PlatformIO CMake/Ninja/ARM GCC | LEXIDE-Ω付属OpenOCD/GDB + MCU-Link |
| macOS | Bash + PlatformIO CMake/Ninja/ARM GCC | pyOCD + PlatformIO ARM GDB + MCU-Link |

`.vscode/launch.json`は1つの`PlantDoctor: MCU-Link (build, flash, debug)`構成を持ち、
WindowsではLEXIDE/OpenOCD、macOSではpyOCDへ自動的に切り替わります。

## リポジトリ取得後の共通セットアップ

CMSIS Coreは`CMSIS_6`をGit submoduleとして
`firmware/ml63q2557/external/CMSIS/`へ配置します。

新規cloneでは次のどちらかを使用してください。

```bash
git clone --recurse-submodules <repository-url>
```

既にclone済みの場合はリポジトリルートで次を実行します。

```bash
git submodule update --init --recursive
```

CMakeでは通常、次のパスをCMSIS Coreとして使用します。

```text
firmware/ml63q2557/external/CMSIS/CMSIS/Core/Include
```

`CMakePresets.json`の共通presetから`CMSIS_CORE_INCLUDE`へこのパスを渡すため、
Windows／macOSで同じpreset名`debug`、`release`を使用できます。

## Windows 初回セットアップ

前提:

- LEXIDE-Ω 2.2.0
- ROHM ML63Q25x7_DFP 1.1.0

リポジトリルートで次を実行します。

```powershell
powershell -ExecutionPolicy Bypass -File .\firmware\ml63q2557\scripts\setup-toolchain.ps1
```

WindowsのデバッグではLEXIDE-Ω付属の`openocd_arm.exe`と`arm-none-eabi-gdb.exe`を使用します。
`flash.ps1`はインストール済みROHM DFPを参照してMCU-Linkから書き込みます。

## macOS 初回セットアップ

### 1. PlatformIO側のビルドツール

`build.sh`は次のPlatformIOパッケージを使用します。

```text
~/.platformio/packages/tool-cmake/
~/.platformio/packages/tool-ninja/
~/.platformio/packages/toolchain-gccarmnoneeabi/
```

Bashスクリプトに実行権限を付与します。

```bash
chmod +x firmware/ml63q2557/scripts/*.sh
```

### 2. pyOCD

macOSのFlash／デバッグバックエンドにはpyOCDを使用します。
`pipx`経由でのインストールを推奨します。

```bash
brew install pipx
pipx ensurepath
pipx install pyocd
```

確認:

```bash
pyocd --version
./firmware/ml63q2557/scripts/pyocd-macos.sh list
```

MCU-Link接続時は、例えば次のようにCMSIS-DAP probeが表示されます。

```text
NXP Semiconductors MCU-LINK (...) CMSIS-DAP
```

リポジトリ内のpyOCD操作には`pyocd-macos.sh`を使用します。このwrapperはローカルDFPと
ML63Q25x7用user scriptを読み込み、MCU-LinkがCMSIS-DAP HID interfaceをusage page
`0xFFEB`で公開する場合の互換処理も適用します。互換処理はNXP MCU-Link
（VID/PID `1FC9:0143`）だけに限定しています。

### 3. ROHM Device Family Pack

ROHM `ML63Q25x7_DFP 1.1.0`をMacへ配置したあと、リポジトリルートで
`setup-toolchain.sh`へDFPのルートを渡します。

例:

```bash
./firmware/ml63q2557/scripts/setup-toolchain.sh \
  --rohm-pack-root "/Users/<user>/ROHM/ML63Q25x7_DFP/1.1.0"
```

セットアップ後、macOSで使用するDFPは次へコピーされます。

```text
firmware/ml63q2557/.tooling/rohm-pack/
```

このディレクトリにはSVD、PDSC、Flash Loader（`.FLM`）などを含みます。
`.tooling/`はローカルツール用なのでGit管理対象外にしてください。

## ビルド

VS Codeでは`Ctrl+Shift+B`でDebugビルドを実行できます。

### Windows

```powershell
# Debug
powershell -ExecutionPolicy Bypass -File .\firmware\ml63q2557\scripts\build.ps1 -Preset debug

# Release
powershell -ExecutionPolicy Bypass -File .\firmware\ml63q2557\scripts\build.ps1 -Preset release
```

### macOS

リポジトリルートから実行します。

```bash
# Debug
./firmware/ml63q2557/scripts/build.sh --preset debug

# Release
./firmware/ml63q2557/scripts/build.sh --preset release
```

`build.sh`がPlatformIO配下のCMake、Ninja、`arm-none-eabi-gcc`を検出してPATHへ追加します。
そのため、macOSでPlatformIOのツールを利用する場合は、通常はCMakeを直接起動せず
`build.sh`またはVS Code Taskを使用してください。

成果物はそれぞれ次へ生成されます。

```text
firmware/ml63q2557/build/debug/
firmware/ml63q2557/build/release/
```

主な成果物:

```text
PlantDoctor.elf
PlantDoctor.hex
PlantDoctor.bin
PlantDoctor.map
compile_commands.json
```

## CMSIS Core

CMakeでは`CMSIS_CORE_INCLUDE`を優先してCMSIS Coreヘッダを使用します。
通常は`CMakePresets.json`から次が設定されます。

```text
${sourceDir}/external/CMSIS/CMSIS/Core/Include
```

macOSで次のエラーが出た場合は、まずsubmoduleを確認してください。

```text
CMSIS Core headers were not found
```

```bash
git submodule update --init --recursive
ls firmware/ml63q2557/external/CMSIS/CMSIS/Core/Include/cmsis_compiler.h
```

## MCU-Linkで書き込み

DT-EBML63Q2557を給電し、MCU-LinkのSWDIO、SWCLK、GND、ターゲット基準電圧を
接続します。基板がUSB給電されている場合、MCU-Linkのターゲット電源出力は使用しません。

VS Codeでは次を使用します。

```text
Terminal > Run Task > Flash: Debug (MCU-Link)
```

### Windows

`flash.ps1`からLEXIDE-Ω／ROHM DFPの書き込み環境を使用します。

### macOS

`flash.sh`から`pyocd-macos.sh`を経由してpyOCDを使用します。

```bash
./firmware/ml63q2557/scripts/flash.sh --preset debug
```

macOSではROHM DFPのML63Q25x7 Flash定義に対して
`firmware/ml63q2557/scripts/pyocd_user_ml63q25x7.py`を読み込みます。

ROHM DFPではFlash programming algorithmが`0x10000000`に定義されていますが、
`ML63Q25x7.FLM`内部のFlash baseは`0x00000000`です。そのままpyOCDへ渡すと
ELFの`0x10000000`セクションをFlash領域として認識できないため、user scriptで次の補正を行います。

```text
実行側ROM alias:        0x00000000-
Flash programming:      0x10000000-0x1003FFFF
```

また、packが自動生成する`0x00000000`側の重複FlashRegionのみを削除し、
実行用`IROM1`は保持します。

同じuser scriptは、Flash algorithmの実行中も動作し続けるwatchdogを8秒へ延長し、
読み出し・sector erase・page programの前にserviceします。これにより、アプリケーションの
通常設定である2秒watchdogがFlash処理中にMCUをresetしてSWD通信を切る問題を回避します。

正常な書き込みではpyOCDから、例えば次のように0より大きいprogrammed byte数が表示されます。
同じELFを書き込み済みの場合は`programmed 0 bytes`かつ`identical N bytes`でも正常です。

```text
Erased 22528 bytes (...)
programmed 22528 bytes (...)
```

または再書き込み時:

```text
Erased 0 bytes (...)
programmed 0 bytes (...)
identical 23552 bytes (...)
```

## VS Codeデバッグ

Run and Debugから次の構成を選択して`F5`を押します。

```text
PlantDoctor: MCU-Link (build, flash, debug)
```

この1つの構成でOS別バックエンドへ切り替わります。

### Windows

LEXIDE-Ω付属OpenOCD/GDBを使用します。

### macOS

Cortex-Debug + pyOCD + PlatformIO ARM GDBを使用します。基本シーケンスは次のとおりです。

```text
Build: Debug
  -> pyOCD GDB server起動
  -> GDB接続
  -> monitor halt
  -> load
  -> emulated reset + halt
  -> mainまで実行して停止
```

ML63Q25x7のCMSIS-Pack `ResetSystem` sequenceは、環境によってSWD通信が
`WAIT ACK`で失敗するため使用しません。macOS側では`reset_type=emulated`を設定し、
Flash taskでは`load.pre_reset=off`／`load.post_reset=off`として不要なResetSystemを回避します。

## macOSトラブルシュート

### `flash driver 'lapis_mcu' not found`

Homebrewの標準OpenOCDにはROHM/LAPIS用`lapis_mcu` Flash driverが含まれていません。
macOSではOpenOCDではなくpyOCDを使用してください。

### `no memory region defined for address 0x10000000`

`pyocd_user_ml63q25x7.py`が読み込まれていないか、ROHM DFPが正しく準備されていません。
`setup-toolchain.sh --rohm-pack-root ...`を再実行し、次を確認してください。

```text
firmware/ml63q2557/.tooling/rohm-pack/Flash/ML63Q25x7.FLM
firmware/ml63q2557/scripts/pyocd_user_ml63q25x7.py
```

### `Overlapping regions in memory map`

古いuser scriptではDFPが作る`0x00000000` FlashRegionとIROM1が重複していました。
現在のuser scriptは誤ったFlashRegionのみ削除します。最新のscriptへ更新してください。

### `ResetSystem ... SWD/JTAG communication failure (WAIT ACK)`

ML63Q25x7のpack reset sequenceによるものです。現在の`flash.sh`と`launch.json`は
pre/post resetを抑止し、emulated resetを使用して回避します。

### Erase／program中の`SWD/JTAG communication failure (WAIT ACK)`

Flash algorithmの実行中にアプリケーションの2秒watchdogが発火すると、MCUがresetして
SWD通信が切れます。現在の`pyocd_user_ml63q25x7.py`はdebug接続中のwatchdogを8秒へ延長し、
Flash操作の区切りごとにserviceします。必ず`flash.sh`または`pyocd-macos.sh`経由で実行してください。

### pyOCDからMCU-Linkが見えない

USB接続直後は列挙に時間がかかる場合があります。数秒待ってから再実行してください。

```bash
./firmware/ml63q2557/scripts/pyocd-macos.sh list
```

MCU-Link firmware V3.172ではCMSIS-DAP HID interfaceがusage page `0xFFEB`で列挙され、
pyOCD 0.45.1の標準filterで除外される場合があります。`pyocd-macos.sh`はこの組み合わせを
互換処理するため、直接`pyocd list`を実行せずwrapperを使用してください。それでも表示されない場合は
USBケーブル、MCU-Link、ターゲット給電を確認します。

## テンプレートとの差分

参照テンプレートはML63Q2537向けですが、このプロジェクトは実機に合わせて
`ML63Q2557`を定義します。テンプレートのサンプル`main.c`やドライバで既存実装を
置き換えず、Plant Doctorの38翻訳単位をそのままCMakeターゲットにしています。

Solist-AIのプリビルドライブラリは、現在の`PlantAi`がまだスタブでAPIを使用して
いないため、この移行ではコピーしていません。利用を開始するときは、参照
テンプレートのライセンスとROHMファイルの再配布条件を確認してから追加します。

また、`firmware/ml63q2557/CommonFiles/`など既存のROHM提供ソースにも利用条件があります。
GitHubへ移す場合は非公開リポジトリを基本とし、公開前に各ファイルの通知と
再配布権限を確認してください。
