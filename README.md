# Plant Doctor ファームウェア

データ・テクノ製DT-EBML63Q2557ボード（ROHM ML63Q2557、Arm Cortex-M0+）用の
ベアメタルBring-upファームウェアです。現在の標準開発環境は
**VS Code + CMake + ARM GNU Toolchain**です。CMSIS-DAP/SWD経由のMCU-Linkで
書き込み・デバッグできます。LEXIDE-Ωプロジェクトも移行時の比較用に残しています。

```text
firmware/
  ml63q2557/                     ML63Q2557向けソースとビルド設定
    CommonFiles/                 ベンダー提供IODriverモジュール（未変更）
    PlantDoctorWorkspace/        アプリ本体と旧LEXIDE-Ωプロジェクト
    CMakeLists.txt               VS Code／ARM GCC用ビルド定義
    CMakePresets.json            Debug／Releaseプリセット
    cmake/                       ARM GNU Toolchain定義
    scripts/                     ビルド、書き込み、ツールチェーン設定
.vscode/                         ビルド、書き込み、デバッグ設定
docs/                            README以外の設計・検証ドキュメント
```

初回セットアップ、ビルド、書き込み、デバッグの手順は、最初に
[VS Code開発環境](docs/VSCODE_SETUP.md)を参照してください。ファームウェアの
動作と実機確認は
[PlantDoctor README](firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/README.md)、ビルド検証結果は
[BUILD_VALIDATION.md](docs/BUILD_VALIDATION.md)
に記録しています。

元のベンダーサンプルは、PlantDoctorがサンプルのソースツリーに依存せず
ビルドできることを確認した後に削除しました。Gitコミット`59c6f58`から復元できます。
比較結果と削除理由は
[SAMPLE_EVALUATION.md](docs/SAMPLE_EVALUATION.md)
に記録しています。
