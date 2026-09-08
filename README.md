# Plant Doctor ファームウェア

データ・テクノ製DT-EBML63Q2557ボード（ROHM ML63Q2557、Arm Cortex-M0+）用の
ベアメタルファームウェアです。標準開発環境は **VS Code + CMake + ARM GNU Toolchain**で、
CMSIS-DAP/SWD経由のMCU-Linkから書き込み・デバッグできます。

```text
firmware/
  ml63q2557/                     ML63Q2557向けソースとビルド設定
    CommonFiles/                 ベンダー提供IODriverモジュール（未変更）
    PlantDoctorWorkspace/        アプリ本体
    CMakeLists.txt               VS Code／ARM GCC用ビルド定義
    CMakePresets.json            Debug／Releaseプリセット
    cmake/                       ARM GNU Toolchain定義
    scripts/                     ビルド、書き込み、ツールチェーン設定
.vscode/                         ビルド、書き込み、デバッグ設定
docs/                            README以外の設計・配線・開発ドキュメント
```

初回セットアップ、ビルド、書き込み、デバッグは
[VS Code開発環境](docs/VSCODE_SETUP.md)を参照してください。

ファームウェアの構造、タイミング、センサー、LCD表示は
[ML63Q2557ファームウェア設計](docs/firmware/ml63q2557/ARCHITECTURE.md)に、
配線は[ピン接続](docs/PIN_CONNECTIONS.md)に、今後の機能追加は
[実行計画](docs/IMPLEMENTATION_PLAN.md)にまとめています。
