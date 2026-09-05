# Plant Doctor ファームウェア

データ・テクノ製DT-EBML63Q2557ボード（ROHM ML63Q2557、Arm Cortex-M0+）用の
ベアメタルBring-upファームウェアです。現在の標準開発環境は
**VS Code + CMake + ARM GNU Toolchain**です。CMSIS-DAP/SWD経由のMCU-Linkで
書き込み・デバッグできます。LEXIDE-Ωプロジェクトも移行時の比較用に残しています。

```text
CommonFiles/                     ベンダー提供IODriverモジュール（未変更）
PlantDoctorWorkspace/
  PlantDoctor/                   アプリ本体と旧LEXIDE-Ωプロジェクト
CMakeLists.txt                   VS Code／ARM GCC用ビルド定義
CMakePresets.json                Debug／Releaseプリセット
.vscode/                         ビルド、書き込み、デバッグ設定
PROTOTYPE_PLAN.md                製品仕様と実装要件
```

初回セットアップ、ビルド、書き込み、デバッグの手順は、最初に
[VS Code開発環境](docs/VSCODE_SETUP.md)を参照してください。ファームウェアの
動作と実機確認は
[PlantDoctor README](PlantDoctorWorkspace/PlantDoctor/README.md)、ビルド検証結果は
[BUILD_VALIDATION.md](PlantDoctorWorkspace/PlantDoctor/docs/BUILD_VALIDATION.md)
に記録しています。

元のベンダーサンプルは、PlantDoctorがサンプルのソースツリーに依存せず
ビルドできることを確認した後に削除しました。Gitコミット`59c6f58`から復元できます。
比較結果と削除理由は
[SAMPLE_EVALUATION.md](PlantDoctorWorkspace/PlantDoctor/docs/SAMPLE_EVALUATION.md)
に記録しています。
