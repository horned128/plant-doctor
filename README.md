# Plant Doctor ファームウェア

データ・テクノ製DT-EBML63Q2557ボード（ROHM ML63Q2557、Arm Cortex-M0+）用の
ベアメタルBring-upファームウェアです。LEXIDE-Ωでのビルドと、
CMSIS-DAP/SWD経由のMCU-Linkによる書き込み・デバッグに対応します。

```text
CommonFiles/                     ベンダー提供IODriverモジュール（未変更）
PlantDoctorWorkspace/
  PlantDoctor/                   LEXIDE-Ωプロジェクト
CODEX.md                         製品仕様と実装要件
```

インポート、ビルド、書き込み、実機確認の手順は、最初に
[PlantDoctorWorkspace/PlantDoctor/README.md](PlantDoctorWorkspace/PlantDoctor/README.md)
を参照してください。ビルド検証結果は
[BUILD_VALIDATION.md](PlantDoctorWorkspace/PlantDoctor/BUILD_VALIDATION.md)
に記録しています。

元のベンダーサンプルは、PlantDoctorがサンプルのソースツリーに依存せず
ビルドできることを確認した後に削除しました。Gitコミット`59c6f58`から復元できます。
比較結果と削除理由は
[SAMPLE_EVALUATION.md](PlantDoctorWorkspace/PlantDoctor/SAMPLE_EVALUATION.md)
に記録しています。
