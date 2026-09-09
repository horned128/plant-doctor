# Plant Doctor ファームウェア

データ・テクノ製DT-EBML63Q2557ボード（ROHM ML63Q2557、Arm Cortex-M0+）用の
ベアメタルファームウェアです。標準開発環境は **VS Code + CMake + ARM GNU Toolchain** で、
Windows／macOSの両方からCMSIS-DAP/SWD経由のMCU-Linkを使用してビルド、書き込み、デバッグできます。

```text
firmware/
  ml63q2557/                     ML63Q2557向けソースとビルド設定
    CommonFiles/                 ベンダー提供IODriverモジュール（未変更）
    PlantDoctorWorkspace/        アプリ本体
    CMakeLists.txt               VS Code／ARM GCC用ビルド定義
    CMakePresets.json            Debug／Releaseプリセット
    cmake/                       ARM GNU Toolchain定義
    external/CMSIS/              CMSIS_6 Git submodule
    scripts/                     Windows／macOS用ビルド・書き込み・セットアップ
    .tooling/                    ローカルROHM DFP（Git管理対象外）
.vscode/                         OS別ビルド、書き込み、デバッグ設定
docs/                            README以外の設計・配線・開発ドキュメント
```

開発フローはOSごとに次の構成です。

- **Windows**: PowerShell + LEXIDE-Ω付属OpenOCD/GDB + MCU-Link
- **macOS**: Bash + PlatformIO提供CMake/Ninja/ARM GCC + pyOCD + MCU-Link
- **共通**: `Ctrl+Shift+B`でDebugビルド、`F5`でビルド・書き込み・`main`停止、`Flash: Debug (MCU-Link)`で書き込みのみ

macOSではROHM ML63Q25x7 DFPのFlash定義をpyOCD用user scriptで補正しています。
DFPのFlash Programming Address `0x10000000`とFLM内部のFlash base `0x00000000`の差異を吸収し、
実際の書き込み領域を`0x10000000-0x1003FFFF`として扱います。また、ML63Q25x7の`ResetSystem`で
SWD `WAIT ACK`が発生するため、Flash前後の不要なresetを抑止し、VS Codeデバッグではemulated resetを使用します。

初回セットアップ、ビルド、書き込み、デバッグの詳細は
[VS Code開発環境](docs/VSCODE_SETUP.md)を参照してください。

ファームウェアの構造、タイミング、センサー、LCD表示は
[ML63Q2557ファームウェア設計](docs/firmware/ml63q2557/ARCHITECTURE.md)に、
配線は[ピン接続](docs/PIN_CONNECTIONS.md)に、今後の機能追加は
[実行計画](docs/IMPLEMENTATION_PLAN.md)にまとめています。
