# ML63Q2557 Plant Doctor ファームウェア設計

## 目的と適用範囲

この文書は、DT-EBML63Q2557（ROHM ML63Q2557、Arm Cortex-M0+）で動作する
Plant Doctorファームウェアの設計書です。現在のソース構造、実行モデル、センサー、
LCD表示、エラー処理、将来拡張の境界を定義します。

VSCodeでの開発・運用手順は[VS Code開発環境](../../VSCODE_SETUP.md)を、配線は
[ピン接続](../../PIN_CONNECTIONS.md)を参照してください。

## 構成と責務

```text
src/main.c
  -> app/App
       -> app/AppStateMachine
       -> board/Board
       -> sensors/SensorManager
       -> ai/PlantAi                 （将来のAI境界）
       -> actuator/PumpControl       （将来の給水制御境界）
       -> storage/PlantLog           （将来の記録境界）

ui/LcdUi
  -> drivers/Lcd
       -> drivers/LcdI2cf0

sensors/EnvironmentSensor, LeafTemperatureSensor
  -> sensors/I2cBus
```

| 層 | 主なディレクトリ | 責務 |
| --- | --- | --- |
| 起動 | `src/`、`startup_ML63Q25x7.c`、`system_ML63Q25x7.c` | リセット後の初期化と`main`の起動。 |
| アプリケーション | `app/` | スーパーループ、状態遷移、周期イベントの振り分け。 |
| 基板制御 | `board/` | クロック、ウォッチドッグ、Timer0、電源自己保持、LED、スイッチを抽象化。 |
| 表示 | `ui/`、`drivers/` | LCDの画面構成、LCDコマンド、I2CF0によるLCD転送。 |
| センサー | `sensors/` | センサー取得、単位変換、妥当性判定、最新スナップショットの提供。 |
| 給水制御 | `actuator/` | SSR（OUT0 / P66）による給水ポンプ制御、安全タイマ（2.0秒）、手動給水操作。 |
| 将来拡張 | `ai/`、`storage/` | AI判定、ログの接続点。現時点では実装範囲を限定する。 |
| ベンダー共通部 | `firmware/ml63q2557/CommonFiles/` | I/Oドライバ、電源、タイマなど。アプリケーション固有コードから変更しない。 |

アプリケーション層は`board/`、`sensors/`、`ui/`のAPIへ依存します。ML63Q2557の
レジスタ操作は、基板制御、LCDドライバ、I2C/ADCを扱うセンサーモジュールに閉じ込めます。
動的メモリ確保は使用しません。

## 実行モデル

`main`は初期化後に`App_RunOnce()`を繰り返すスーパーループです。Timer0割り込みは
10 ms Tickの記録だけを行い、LCD、I2C、ADC、センサー取得などの時間を要する処理は
すべてmainコンテキストで実行します。各待機処理には上限を設けます。

```text
main
  -> App_Init
       -> AppStateMachine_Init
       -> Board_Init
       -> SensorManager_Init / PlantAi_Init / PumpControl_Init / PlantLog_Init
  -> App_RunOnce を繰り返す
       -> ウォッチドッグをサービス
       -> Tickあふれをエラーとして検出
       -> 10 ms Tickごとに Board / SensorManager / AI / Log / 状態機械を更新
       -> LCD表示を含む状態機械を処理
```

### 状態機械

| 状態 | 動作 | 次の状態 |
| --- | --- | --- |
| `BOOT` | LCDを初期化し、`PLANT DOCTOR` / `BOARD TEST`を表示する。 | 成功時は`SELF_TEST`、失敗時は`ERROR`。 |
| `SELF_TEST` | 電源自己保持を確認する。 | 500 ms経過で`MONITOR`、電源異常時は`ERROR`。 |
| `MONITOR` | スイッチ、LED、センサー取得、LCDページ切替を処理する。単発のLCD I/O失敗時は表示を停止して復旧を試みる。 | 復旧成功時は`MONITOR`を継続し、3回連続で復旧に失敗した場合は`ERROR`。 |
| `ERROR` | LCDが使用可能な場合にエラー名を表示し、3個のLEDで交互点滅する。 | リセットまたは再起動まで継続。 |

### 周期と時間単位

`PLANT_DOCTOR_TICK_MS`は10 msです。表示更新を遅くしても、センサー取得周期は変わりません。

| 設定 | Tick数 | 周期 | 用途 |
| --- | ---: | ---: | --- |
| `PLANT_DOCTOR_SELF_TEST_TICKS` | 50 | 500 ms | 起動時の自己確認表示時間。 |
| `PLANT_DOCTOR_SENSOR_SAMPLE_TICKS` | 100 | 1秒 | センサーの取得、スナップショット更新、表示中センサーページの再描画。 |
| `PLANT_DOCTOR_SENSOR_DISPLAY_TICKS` | 500 | 5秒 | LCDのセンサーページを次のページへ切り替える間隔。 |
| `PLANT_DOCTOR_LCD_RECOVERY_INTERVAL_TICKS` | 100 | 1秒 | LCD復旧の再試行間隔。 |
| `PLANT_DOCTOR_LCD_RECOVERY_MAX_ATTEMPTS` | 3 | - | LCD復旧の連続試行上限。 |
| `PLANT_DOCTOR_LED_BLINK_TICKS` | 100 | 1秒 | `MONITOR`中のLED1ハートビート。 |
| `PLANT_DOCTOR_ERROR_BLINK_TICKS` | 25 | 250 ms | `ERROR`中のLED交互点滅。 |
| `PLANT_DOCTOR_SWITCH_DEBOUNCE_POLLS` | 2 | 20 ms | スイッチ入力のデバウンスに必要な連続ポーリング回数。 |

表示中のセンサーページは、最新スナップショットを使って1秒ごとに再描画します。ページ番号だけを
0 → 1 → 2 の順に5秒ごとに切り替えるため、3ページ全体の一巡は約15秒です。

LCDの転送に一度失敗すると、バックライトを消してLCDとI2CF0を再初期化します。最初の再初期化だけは
直後に試行し、それ以降の失敗時は1秒ごとに、合計3回まで試行します。この間もセンサー取得、スイッチ処理、LED1の
ハートビート、ページ時刻処理を継続します。復旧成功は、再初期化だけでなく直近のセンサーページを
再描画できた時点で確定します。再初期化後の描画が失敗した場合も試行回数を維持し、3回連続で失敗した
場合だけ`ERROR LCD`へ遷移します。

## センサーとデータ経路

`SensorManager`は1秒ごとに最新値を`PLANT_SENSOR_SNAPSHOT`へ集約します。状態機械は同じ周期で
表示中のLCDセンサーページをこのスナップショットから再描画します。センサー取得に失敗した値は
有効値として扱わず、LCDには`--`を表示します。1台のI2Cセンサーが応答しなくても、他の取得処理と
周期処理を継続します。

| モジュール | 部品 | 接続・方式 | スナップショットの値 | 単位・有効条件 |
| --- | --- | --- | --- | --- |
| `LeafTemperatureSensor` | SEN0206 / MLX90614 | I2CF0、7 bitアドレス`0x5A` | `leafTemperatureCentiC` | 摂氏の100分の1。PECとセンサーエラーフラグを確認する。 |
| `EnvironmentSensor` | BME280 | I2CF0、7 bitアドレス`0x76`または`0x77` | `airTemperatureCentiC`、`relativeHumidityCentiPercent`、`barometricPressurePa` | 摂氏・相対湿度の100分の1、気圧(Pa)。トリミング補正済みのときだけ有効。 |
| `EnvironmentSensor` | SEN0228 / VEML7700 | I2CF0、7 bitアドレス`0x10` | `illuminanceCentiLux`、`illuminanceRaw` | 照度の100分の1 luxと生値。初回に設定を書き込む。 |
| `SoilMoistureSensor` | SEN0193 | ADC0 | `soilMoistureRaw` | 0～4095の相対値。CN6の反転増幅回路に合わせて0を乾燥側、4095を湿潤側とする。水分率ではない。 |
| `TankLevelSensor` | SEN0204 | CN5 IN0、Lowアクティブ入力 | `tankLiquidDetected` | `true`は液面検出。通信妥当性を示す値ではない。 |

LCDとSEN0206、BME280、SEN0228はCN3の同じI2CF0バスを共有します。LCDドライバは
LCD用の転送アドレス`0x7C`を使用し、センサーバスAPIは7 bitアドレスを受け取ります。
すべてmainコンテキストで同期転送し、バス待機、転送完了、NACKを上限付きで判定します。

## LCD表示リファレンス

LCDは16文字×2行です。文字列は`ui/LcdUi.c`で生成し、短い文字列の後ろは空白で消去します。
各センサーページは5秒間表示したまま、表示内容だけを1秒ごとに更新します。

### 起動・スイッチ表示

| 表示 | 行 | 意味 |
| --- | --- | --- |
| `PLANT DOCTOR` | 1行目 | 製品名。起動表示とエラー表示の見出し。 |
| `BOARD TEST` | 2行目 | 起動時の基板確認表示。スイッチをすべて離した時のスイッチ表示にも使う。 |
| `SW1 PRESSED` | 2行目 | SW1が押されていることを示す。 |
| `SW2 PRESSED` | 2行目 | SW2が押されていることを示す。 |
| `SW3 PRESSED` | 2行目 | SW3が押されていることを示す。 |
| `SW4 PRESSED` | 2行目 | SW4が押されていることを示す。複数同時押し時は番号の小さいスイッチを優先して表示する。 |

`MONITOR`中は、LED1が1秒周期で点滅します。LED2はSW1/SW2、LED3はSW3/SW4が押されている間に点灯します。

### センサーページ

| ページ | 1行目 | 2行目 | 表示内容 |
| --- | --- | --- | --- |
| 0 | `AIR:+25.00C` | `HUM:50.00%` | BME280の気温と相対湿度。温度は符号付き摂氏、湿度は百分率。 |
| 1 | `LEAF:+24.50C` | `SOIL:1234` | SEN0206の葉温とSEN0193の土壌水分生値。`SOIL`は未校正の相対値。 |
| 2 | `LUX:123.45` | `TANK:WET` | SEN0228の照度（lux）とSEN0204の液面検出状態。液面未検出時は`TANK:EMPTY`。 |

`AIR:--`と`HUM:--`、`LEAF:--`、`SOIL:--`、`LUX:--`は、それぞれ対応するセンサー値が
今回の取得で有効でないことを示します。`TANK:EMPTY`は液面を検出していない状態であり、
センサー未接続を判定する表示ではありません。

### エラー表示

エラー時の1行目は`PLANT DOCTOR`、2行目は次のいずれかです。

| LCD表示 | `PLANT_DOCTOR_ERROR` | 意味 |
| --- | --- | --- |
| `ERROR POWER` | `PLANT_DOCTOR_ERROR_POWER` | 電源自己保持または電源制御の初期化失敗。 |
| `ERROR TIMER` | `PLANT_DOCTOR_ERROR_TIMER` | Timer0の初期化失敗。 |
| `ERROR SWITCH` | `PLANT_DOCTOR_ERROR_SWITCH` | スイッチ入力処理の失敗。 |
| `ERROR LCD` | `PLANT_DOCTOR_ERROR_LCD_INIT` / `PLANT_DOCTOR_ERROR_LCD_IO` | LCDの初期化または通信失敗。 |
| `ERROR TIMING` | `PLANT_DOCTOR_ERROR_TICK_OVERFLOW` | mainループがTickを処理しきれず、Tickがあふれた。 |
| `ERROR SENSOR` | `PLANT_DOCTOR_ERROR_SENSOR_INTERFACE` | センサー初期化の失敗。 |
| `ERROR STORAGE` | `PLANT_DOCTOR_ERROR_STORAGE_INTERFACE` | 記録機能の初期化失敗。 |
| `ERROR UNKNOWN` | `PLANT_DOCTOR_ERROR_NONE`または未定義値 | エラー種別を特定できない。 |

`ERROR`中はLED1とLED3が同じ位相、LED2が反対位相で250 msごとに交互点滅します。

## 給水制御と将来拡張の境界

`PumpControl`はCN5 OUT0（P66、Nchオープンドレイン出力）を介してDFR0457/外部12V給水ポンプを
安全に制御します。1回の最大駆動時間（2.0秒）による自動停止、給水後クールダウン（3.0秒）、
SW4押下による手動給水・停止トリガー、LCD表示フィードバック、エラー時緊急遮断を提供します。

`PlantAi`はセンサー値から植物状態を判定する境界、`PlantLog`は時刻付き測定値とイベントを記録する
境界です。将来機能はこれらのモジュールを拡張し、センサー取得、LCD、基板制御、給水制御の既存責務を
直接混在させません。

実装の優先順位と完了条件は[Plant Doctor 実行計画](../../IMPLEMENTATION_PLAN.md)を参照してください。
