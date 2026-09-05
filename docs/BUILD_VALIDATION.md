# ビルド検証

## VS Code／ARM GCC移植検証（2026-09-04）

`solist_ai_project_template`の構成を参考に追加した
`firmware/ml63q2557/CMakeLists.txt`を、
次の環境で検証しました。

- VS Code + CMake Tools
- CMake 3.30.2
- Ninja 1.9.0
- Arm GNU Toolchain 14.2.Rel1（GCC 14.2.1）
- ARM CMSIS 6.3.0
- ターゲット：ML63Q2557 / Arm Cortex-M0+

既存の全38翻訳単位にGCC用newlibシステムコール1翻訳単位を加え、Debugと
Releaseを警告0・エラー0でビルドしました。

| 構成 | 最適化 | 失敗 | 警告 | `text` | `data` | `bss` | `dec` |
|---|---:|---:|---:|---:|---:|---:|---:|
| Debug | `-O0`, DWARF 4 | 0 | 0 | 13,600 | 24 | 112 | 13,736 |
| Release | `-Os` | 0 | 0 | 9,148 | 24 | 108 | 9,280 |

ELFと生成物について次を確認しました。

- ARM EABI5、soft-float、Cortex-M0+用ELFである
- エントリポイントは`Reset_Handler`（`0x00000201`）である
- 強いシンボルの`TM0_IRQHandler`と`TM1_IRQHandler`がリンクされている
- `.codeoption`は`0x1003FFC0`に64バイト配置されている
- Debug／ReleaseともELF、Intel HEX、BIN、MAPを生成する
- LEXIDE付属`openocd_arm.exe` 0.12.0が、CMSIS-DAP設定とROHM DFPの
  `ml63q25x7.cfg`をエラーなく読み込む

実機フラッシュはこの移植作業では実行していません。既存の実機確認済み
LEXIDE成果物を残したまま、VS Code成果物を
`firmware/ml63q2557/build/`以下へ分離しています。

## 検証環境

- LEXIDE-Ω 2.2.0
- LEXIDE Arm BuildTools `Ver.20260317`
- ARM CMSIS 5.9.0
- ROHM ML63Q25x7_DFP 1.1.0
- ターゲット：ML63Q2557 / Arm Cortex-M0+

## 結果

PlantDoctorとリンクされたCommonFilesを構成する全38個のC翻訳単位を、
`-mcpu=cortex-m0plus`を指定してクリーンな状態からコンパイルしました。
生成されたアセンブリをアセンブルし、プロジェクトのリンカスクリプトと
LEXIDEランタイムライブラリでリンクして、Intel HEXへ変換しました。

| 構成 | 最適化 | 失敗 | 警告 | `text` | `data` | `bss` | `dec` |
|---|---:|---:|---:|---:|---:|---:|---:|
| Debug | `-O0`, DWARF 4 | 0 | 0 | 12,407 | 4,100 | 100 | 16,607 |
| Release | `-O2` | 0 | 0 | 9,552 | 4,104 | 104 | 13,760 |

サイズには、リンカスクリプトで予約したヒープ領域が含まれます。
アプリケーションコードは`malloc`、`calloc`、`realloc`、`free`、
またはC++の動的確保APIを呼び出しません。

両構成のELFを検査し、次の項目を確認しました。

- Cortex-M0+用のArm EABI実行ファイルで、`Reset_Handler`が起動経路になっている
- 強いシンボルとして定義した`TM0_IRQHandler`と`TM1_IRQHandler`がリンクされている
- `.codeoption`が`0x1003FFC0`に正確に64バイト配置されている
- 選択したビルドディレクトリ以下に`PlantDoctor.elf`と`PlantDoctor.hex`が生成される
- 生成された成果物は意図的にGitの追跡対象外としている

## 実機デバッグによる修正（2026-07-20）

最初の実機動作では、Timer0がその後正常にカウントしていたにもかかわらず、
`PLANT_DOCTOR_ERROR_TIMER`によって`APP_STATE_ERROR`へ遷移しました。
`TMSTAT`はLSCLKに同期するため、`timer0_start()`直後には動作状態を返しません。
`BoardTimer_Init()`を修正し、最初の状態読み出しを失敗と判定せず、制限時間付きで
状態変化を待つようにしました。

修正後に、両構成とも全38個のC翻訳単位から再ビルドしました。上表は修正版の
サイズを示しています。その後、修正版を実機へ書き込み、LCD表示とスイッチ入力が
正常に動作することを確認しました。

## LEXIDEヘッドレスビルドに関する注意

LEXIDEにインストールされたArm管理ビルドプラグインは、ターゲットオプションの
評価時にEclipseのグラフィカルWorkbenchへアクセスします。そのため、この
LEXIDEリリースでは、CDTのヘッドレスビルドからmakefile生成を完了できません。
上記のコンパイル／リンク検証では、インストール済みのLEXIDEコンパイラ、
アセンブラ、リンカ、ランタイムライブラリ、デバイスヘッダ、リンカスクリプト、
CMSIS Packを直接使用しました。通常の開発ではGUI版LEXIDEによるインポートと
ビルドを使用し、実機へ書き込む前に開発PC上で一度実行してください。

## デバッグプローブ接続確認

Windows上で、接続したMCU-Linkが`MCU-LINK (r0FB) CMSIS-DAP V3.172`として
認識されることを確認しました。LEXIDE付属のOpenOCD 0.12.0、`cmsis-dap.cfg`、
デバイスパックの`ml63q25x7.cfg`を使用し、500 kHzのSWD接続に成功しました。
DPIDR `0x0BC11477`を読み出し、ブレークポイント4個、ウォッチポイント2個を持つ
Cortex-M0+ r0p1ターゲットを検出しました。この接続確認ではターゲットフラッシュの
消去や書き込みを行っていません。書き込みと実機動作の確認には`README.md`の
手順を使用してください。
