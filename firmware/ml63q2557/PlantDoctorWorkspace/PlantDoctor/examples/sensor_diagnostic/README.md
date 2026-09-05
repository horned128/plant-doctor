# センサ診断ファームウェア

`SensorDiagnostic`は、`docs/solist_ai_terrarium_pin_connections.md`に記載した
センサをすべて接続した状態で、通信可否と測定値を確認するための独立した
ファームウェアです。通常の`PlantDoctor`とは別のELF/HEX/BINとして生成されます。

## 対象

| センサ | 入力 | 表示値 |
|---|---|---|
| SEN0206 / MLX90614 | I2C `0x5A` | 対象物温度、センサ周囲温度 |
| SEN0385 / SHT31 | I2C `0x44` | 気温、相対湿度 |
| SEN0228 / VEML7700 | I2C `0x10` | 照度、RAW値 |
| SEN0193 | CN6 ADC0 | 極性補正済みRAW値、入力換算mV |
| SEN0204 | CN5 IN0 | 液面検出のON/OFF |

I2Cは、SEN0206とSEN0385の長いケーブルを含む構成に合わせて約100kHzで
動作します。SEN0193の値は設置環境ごとの乾燥値・水中値による校正前なので、
診断ファームウェアでは水分率へ変換しません。

## ビルドと書き込み

リポジトリルートから次を実行します。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File firmware/ml63q2557/scripts/build.ps1 -Preset debug

powershell -NoProfile -ExecutionPolicy Bypass `
  -File firmware/ml63q2557/scripts/flash.ps1 `
  -Preset debug -Target SensorDiagnostic
```

VS Codeでは、タスク`Flash: Sensor Diagnostic (MCU-Link)`、またはデバッグ構成
`Sensor Diagnostic: MCU-Link (build, flash, debug)`を使用できます。

通常ファームウェアへ戻す場合は、従来どおり`Flash: Debug (MCU-Link)`を実行します。

## LCDとスイッチ

起動後、約1秒ごとに全センサを読み、約3秒ごとに次のページへ切り替えます。

- SW1: 前のページ
- SW2: 次のページ
- SW3: 自動ページ切り替えの一時停止・再開
- SW4: 即時再測定

I2Cセンサでは`OK`のほかに`NACK`、`BUSY`、`TIMEOUT`、`CRC`、`DATA`を表示します。

SEN0385を接続していない場合、先頭ページの`S:NACK`とSEN0385ページの
`ERROR:NACK`は期待どおりの結果です。SEN0204はI2C機器ではないため、未接続時に
`TANK:OFF`または`LIQUID:NO`となるだけで、通信エラーにはなりません。

## LEDとデバッガ

- LED1点滅: メインループ動作中
- LED2点灯: いずれかの測定でエラー
- LED3点灯: LCD初期化またはLCD更新エラー

SEN0385などがバスをLowへ固定すると、同じバス上のLCDも利用できません。その場合も
プログラムは停止せず、LED1/LED2/LED3と、デバッガのグローバル変数
`g_sensorDiagnosticSnapshot`でセンサ別の状態と取得済みのADC/液面値を確認できます。

LCDのI2C書込みが一度失敗するとLED3が点灯します。診断処理自体は継続し、約1秒ごとに
LCD周辺回路の再初期化と現在ページの再描画を試みます。デバッガでは
`lcdReady`、`currentPage`、`lastLcdStatus`、`lcdFailureCount`、
`lcdRecoveryAttemptCount`、`lcdRecoverySuccessCount`を確認できます。

## 配線条件

- JP1: 3.3V側
- JP4: SHORT
- JP7: SHORT（DCカップリング）
- JP6: 5V側
- JP5: OPEN
- SW6: ゲイン1倍（SW6-1のみON）

ポンプはこの診断ファームウェアでは駆動しません。最初の確認では外部12V電源と
ポンプを外しておくことを推奨します。
