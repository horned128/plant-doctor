# Plant Doctor リポジトリ構成

Plant Doctor は、データ・テクノ製 DT-EBML63Q2557 ボードに搭載された ROHM ML63Q2557 向けのベアメタルファームウェアプロジェクトです。標準開発環境は **VS Code + CMake + ARM GNU Toolchain** です。

Windows と macOS の両方でビルド・書き込み・デバッグできます。

- Windows: PowerShell + LEXIDE-Ω 付属 OpenOCD / GDB
- macOS: Bash + pyOCD + MCU-Link
- CMSIS Core: `firmware/ml63q2557/external/CMSIS` の CMSIS_6 Git submodule
- ROHM DFP: macOS では初回セットアップ時に `firmware/ml63q2557/.tooling/rohm-pack/` へローカル配置
- `.tooling/` と `build/` は生成物・ローカル依存物であり、原則 Git 管理しない

## ルート

```text
plant-doctor/
├── .github/
│   └── workflows/
│       └── build.yml              CI ビルド定義
├── .vscode/                       VS Code のビルド・書き込み・デバッグ設定
├── docs/                          設計・開発・配線ドキュメント
├── firmware/                      マイコン別ファームウェア
├── .gitattributes                 Git の属性設定
├── .gitignore                     Git 管理対象外設定
├── .gitmodules                    CMSIS_6 submodule 定義
├── AGENTS.md                      リポジトリ構成・開発上の前提
└── README.md                      リポジトリ概要
```

## ドキュメント

```text
docs/
├── firmware/
│   ├── CODING_RULES.md            ファームウェアのコーディングルール
│   └── ml63q2557/
│       └── ARCHITECTURE.md        ML63Q2557 ファームウェアの設計書
├── IMPLEMENTATION_PLAN.md         機能実装の計画
├── PIN_CONNECTIONS.md             基板・センサー・LCD のピン接続
├── PROTOTYPE_PLAN.md              プロトタイプ全体の計画
└── VSCODE_SETUP.md                Windows / macOS の VS Code 開発環境
```

VS Code での初回セットアップ、ビルド、Flash、F5 デバッグの具体的な手順は
`docs/VSCODE_SETUP.md` を参照してください。

## ML63Q2557 ファームウェア

```text
firmware/ml63q2557/
├── CMakeLists.txt                 Plant Doctor の CMake ビルド定義
├── CMakePresets.json              Debug / Release のクロスプラットフォームプリセット
├── cmake/
│   └── arm-none-eabi-toolchain.cmake
│                                  ARM GNU クロスコンパイル設定
├── CommonFiles/                   ベンダー提供の共通ドライバ・電源・タイマーモジュール
│   ├── Driver/                    ML63Q2557 周辺機能ドライバ
│   ├── Power/                     電源入力・出力・監視・保持制御
│   ├── SoftSpi/                   ソフトウェア SPI
│   └── Timer/                     共通タイマ制御
├── external/
│   └── CMSIS/                     ARM CMSIS_6 Git submodule
│       └── CMSIS/Core/Include/    CMake が使用する CMSIS Core ヘッダ
├── PlantDoctorWorkspace/          Plant Doctor アプリケーションワークスペース
│   ├── README.md                  ワークスペース概要
│   └── PlantDoctor/               アプリケーション本体
├── platform/
│   └── gcc_syscalls.c             ARM GCC 用のシステムコール補助
├── scripts/
│   ├── build.ps1                  Windows ビルド
│   ├── flash.ps1                  Windows 書き込み
│   ├── setup-toolchain.ps1        Windows ツールチェーン設定
│   ├── build.sh                   macOS ビルド
│   ├── flash.sh                   macOS pyOCD 書き込み
│   ├── setup-toolchain.sh         macOS ツールチェーン / ROHM DFP 設定
│   ├── pyocd-macos.sh             Cortex-Debug から pyOCD を起動するラッパー
│   └── pyocd_user_ml63q25x7.py    ML63Q25x7 の pyOCD メモリマップ補正
├── build/                         CMake ビルド出力（Git 管理外）
│   ├── debug/
│   └── release/
└── .tooling/                      macOS ローカルツール資産（Git 管理外）
    └── rohm-pack/                 setup-toolchain.sh が準備する ROHM DFP
```

### CMSIS

CMSIS Core は `external/CMSIS` の CMSIS_6 Git submodule を使用します。

新しい clone では次を実行してください。

```bash
git submodule update --init --recursive
```

CMake では主に次のパスを使用します。

```text
firmware/ml63q2557/external/CMSIS/CMSIS/Core/Include
```

## PlantDoctor アプリケーション

```text
firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/
├── actuator/                      アクチュエータ制御（ポンプ）
├── ai/                            植物診断 AI・特徴量
├── app/                           アプリケーション本体・状態機械
├── board/                         基板依存処理（LED・スイッチ・電源・タイマ）
├── config/                        アプリケーション設定
├── drivers/                       LCD などのデバイスドライバ
├── inc/                           共通ステータス型などの公開インクルード
├── sensors/                       I2C バス、環境・葉温・土壌水分・液面センサー
├── storage/                       植物ログなどの保存処理
├── ui/                            LCD 表示・画面制御
├── src/
│   └── main.c                     エントリポイント
├── ML63Q25x7.h                    MCU レジスタ定義
├── ML63Q25x7_lccarm.ld            メモリ配置・リンカースクリプト
├── startup_ML63Q25x7.c            スタートアップ処理
├── system_ML63Q25x7.c/.h          システム初期化
├── NmiHandler.c                   NMI ハンドラ
├── codeoption.c/.h                MCU コードオプション
├── codeoption_config.h            コードオプション設定
├── PlantDoctor.tcl                LEXIDE-Ω 由来のプロジェクト情報
└── README.md                      アプリケーション概要
```

## 主要な構成関係

```text
firmware/ml63q2557/CMakeLists.txt
  ├── PlantDoctorWorkspace/PlantDoctor/  アプリケーションソース
  ├── CommonFiles/                       共通・ベンダーソース
  └── external/CMSIS/.../Include         CMSIS Core

PlantDoctor/app/AppStateMachine.c
  ├── board/                             LED・スイッチ・電源・タイマ
  ├── sensors/                           センサー取得と I2C バス
  └── ui/                                LCD 表示と LCD I2C 通信
```

## ビルド

Debug / Release は同じ CMake preset 名を Windows と macOS で使用します。

```text
debug
release
```

成果物は次へ生成します。

```text
firmware/ml63q2557/build/debug/
firmware/ml63q2557/build/release/
```

主な成果物は次のとおりです。

```text
PlantDoctor.elf
PlantDoctor.hex
PlantDoctor.bin
PlantDoctor.map
compile_commands.json
```

VS Code の標準ビルドタスクは `Build: Debug` です。

## MCU-Link 書き込み・デバッグ

### Windows

Windows は既存の LEXIDE-Ω 環境を利用します。

```text
VS Code / Cortex-Debug
  -> LEXIDE-Ω OpenOCD
  -> MCU-Link / CMSIS-DAP
  -> ML63Q2557
```

PowerShell スクリプトを使用します。

```text
build.ps1
flash.ps1
setup-toolchain.ps1
```

### macOS

macOS は pyOCD を使用します。

```text
VS Code / Cortex-Debug
  -> pyOCD
  -> MCU-Link / CMSIS-DAP
  -> ML63Q2557
```

Bash / Python スクリプトを使用します。

```text
build.sh
flash.sh
setup-toolchain.sh
pyocd-macos.sh
pyocd_user_ml63q25x7.py
```

ROHM ML63Q25x7 DFP 1.1.0 は初回セットアップ時に
`firmware/ml63q2557/.tooling/rohm-pack/` へ準備します。

### ML63Q25x7 の pyOCD 補正

ROHM DFP / FLM では、実行時の ROM alias と Flash 書き込みアドレスの扱いが異なります。

```text
実行側 IROM1                 0x00000000
Flash programming region    0x10000000 - 0x1003FFFF
```

`pyocd_user_ml63q25x7.py` は、pyOCD が生成する `0x00000000` 側の重複 FlashRegion を除去し、
ROHM の `ML63Q25x7.FLM` を `0x10000000` 側の書き込み領域として登録します。

この補正は削除しないでください。削除すると、ELF の Flash セクションを pyOCD が
`0x10000000` に書き込めなくなる可能性があります。

また、このデバイスでは CMSIS-Pack の `ResetSystem` シーケンスが MCU-Link / SWD で
`WAIT ACK` になる場合があるため、macOS の pyOCD 経路では pre/post reset を抑制し、
デバッグ時は emulated reset を使用します。

## VS Code タスクとデバッグ

主要タスクは次のとおりです。

```text
Build: Debug
Build: Release
Flash: Debug (MCU-Link)
```

F5 で使用する標準デバッグ構成は次です。

```text
PlantDoctor: MCU-Link (build, flash, debug)
```

この 1 つの構成を OS ごとに切り替えて使用します。

```text
Windows
  -> LEXIDE-Ω / OpenOCD

macOS
  -> pyOCD
```

一時的な検証用や fallback 用の macOS 専用デバッグ構成は、標準構成が正常動作する場合は
`.vscode/launch.json` に残す必要はありません。

### ATOMS3 Lite (ESP32-S3 / PlatformIO) タスク

ATOMS3 Lite ゲートウェイ（`firmware/esp32s3`）は **PlatformIO** を使用してビルド・書き込み・監視を行います。`idf.py` は不要です。

```text
Build: ATOMS3 Lite (PlatformIO)
Flash: ATOMS3 Lite (PlatformIO)
Monitor: ATOMS3 Lite (PlatformIO)
Flash & Monitor: ATOMS3 Lite (PlatformIO)
```

F5 デバッグ構成:
```text
ATOMS3 Lite: PlatformIO Debug
```

## 開発時の注意

- `CommonFiles/` と一部の `PlantDoctorWorkspace/PlantDoctor/` には ROHM / ベンダー由来のファイルがあります。不要な変更を避けてください。
- `external/CMSIS` は Git submodule として扱い、直接プロジェクト固有の修正を入れないでください。
- `build/`、`.tooling/` は生成物・ローカル環境依存物です。ソースとして扱わないでください。
- ROHM DFP のファイルはライセンス・再配布条件を確認し、原則としてリポジトリへコミットしないでください。
- Windows と macOS の両対応を維持してください。一方の OS の絶対パスを共通設定へ直書きしないでください。
- VS Code / CMake 周辺を変更した場合は、少なくとも Debug ビルドと MCU-Link 書き込みを確認してください。
- macOS の pyOCD 対応を変更する場合は、Flash アドレス `0x10000000`、重複 FlashRegion、reset シーケンスを再確認してください。
