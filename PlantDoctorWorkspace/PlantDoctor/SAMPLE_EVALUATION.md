# ベンダーサンプルの評価

9個のサンプルはすべて、同一のML63Q25x7デバイス、スタートアップ、システム、
コードオプション、リンカ関連ファイルを使用しています。各Eclipseプロジェクトは
リポジトリ共通のCommonFilesディレクトリをリンクし、プロジェクト固有のソースで
各ペリフェラルの動作を実装しています。

| サンプル | 主な用途 | CommonFilesへの論理的な依存 | Plant Doctorでの用途 |
|---|---|---|---|
| Lcd | I2CF0接続のキャラクタLCD | Driver、Power、Timer | ベースプロジェクト、LCDのピン・コマンド設定の参照元 |
| PowerControl | POWER_KEEPと電源ボタンによるシャットダウン | Driver、Power、Timer | 電源自己保持シーケンスの参照元 |
| GPIO | LED、4個の押しボタン、DIPスイッチ、リレー／レギュレータ | Driver、Power、Timer | LED、スイッチ、5 V電源制御の参照元 |
| AnalogSensor | ADC0によるバッファ付きアナログサンプリング | Driver、Power | 将来の土壌センサ実装の参照元のみ |
| MemsAccelerometer | SSIOF0接続のKX134 | Driver、Power、Timer | 将来追加する任意の振動入力の参照元のみ |
| Fram | ソフトウェアSPI接続のFRAM | Driver、Power、SoftSpi | 将来のログ保存実装の参照元のみ |
| Rtc | ソフトウェアSPI接続のRX4111 | Driver、Power、SoftSpi | 将来の実時刻処理の参照元のみ |
| Uart | UARTF1割り込みエコー | Driver、Power | 今回のBring-upでは未使用 |
| PowerMonitoringAnalogInput | ADC1による電源電圧測定 | Driver、Power、Timer | 将来の電源監視実装の参照元のみ |

ベースにはLcdサンプルを選定しました。必須ペリフェラルの中でLCD/I2CF0が最も
設定依存性が高く、このサンプルには適切な電源、クロック、Timer1、スタートアップ、
リンカ設定が既に含まれているためです。GPIOとPowerControlの処理は小さく、
Plant Doctorのボードアダプタ内へ分離できます。この方針により、確認済みの
ピン割り当てを維持しながら、複数のEclipseプロジェクトの無理な結合を避けました。

プロジェクト固有のLCDドライバは、サンプルから意図的に2点変更しています。
I2C待機処理に上限を設け、I2CF0割り込み内ではなくmainコンテキストで
バイト転送を行います。CommonFilesは変更していません。

## 削除の判断

DebugとReleaseの検証が完了し、PlantDoctorが`SampleProject/`を参照していない
ことを確認してから、9個のサンプルワークスペースを削除しました。必要な
ピン割り当てと動作はPlantDoctorのアダプタへ反映し、再利用するベンダー実装は
CommonFilesに残しています。

| 削除したワークスペース | 理由 |
|---|---|
| Lcd | 制限時間付きのプロジェクト固有LCDドライバとUIラッパーで置き換えたため |
| PowerControl | POWER_KEEPを`PowerControlAdapter`でラップしたため |
| GPIO | LED、押しボタン、5 V電源機能を`board/`以下でラップしたため |
| AnalogSensor | センサ型式とピンが未確定で、現段階では中立的なインターフェースのみが適切なため |
| MemsAccelerometer | Bring-upには不要で、将来の入力用モジュール境界を用意済みのため |
| Fram | ストレージ用ハードウェアが未確定で、`PlantLog`に拡張境界を用意済みのため |
| Rtc | 実時刻付きログを後回しとし、現在のアプリケーションから参照していないため |
| Uart | FTDI/COM通信がBring-upの対象外として明示されているため |
| PowerMonitoringAnalogInput | 電源監視は将来拡張であり、現在の依存関係に含まれないため |

実行時依存が不明なファイルは削除していません。削除前に、PlantDoctorと
CommonFilesだけからリンクマップが生成されることを確認しました。削除前の
完全なソースはGitコミット`4eebc97`、未変更のベンダー初期状態は`59c6f58`
から復元できます。
