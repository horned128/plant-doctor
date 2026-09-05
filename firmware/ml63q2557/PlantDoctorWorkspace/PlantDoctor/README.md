# Plant Doctor Bring-upファームウェア

このプロジェクトは、データ・テクノ製DT-EBML63Q2557ボード向けの最初の
実機Bring-upファームウェアです。ベアメタルのスーパーループと明示的な
状態機械を使用し、動的メモリ確保は使用しません。

## Bring-up動作

- `POWER_KEEP`と基板上の5 Vレギュレータを有効化する
- Timer0を10 ms周期の自動リロード式システムTickとして動作させる
- `MONITOR`状態でLED1を1秒周期で点滅させる
- LCDを初期化して次の文字列を表示する

  ```text
  PLANT DOCTOR
  BOARD TEST
  ```

- 4個の押しボタンを10 ms周期でデバウンスする。スイッチを押すと
  `SW1 PRESSED`～`SW4 PRESSED`を表示し、LED2/LED3でもスイッチグループを示す
- Bring-upエラーを検出すると`ERROR`へ遷移する。LCDが使用可能な場合は
  エラー名を表示し、LED1/LED2/LED3を250 ms周期で交互点滅させる

Timer0とTimer1の割り込みハンドラは、カウンタまたは完了フラグの更新だけを
行います。LCDのI2C転送はmainコンテキストで同期的に実行し、待機処理には
上限を設けています。この版のセンサ、ポンプ、AI、ストレージモジュールは、
動作を伴わない将来拡張用の境界です。

配線済みセンサの通信と測定値を確認する独立ファームウェアは、
[`examples/sensor_diagnostic/README.md`](examples/sensor_diagnostic/README.md)を参照してください。

## アーキテクチャ

```text
src/main.c
  -> app/App
       -> app/AppStateMachine
       -> board/Board
       -> sensors/SensorManager       （スタブ境界）
       -> ai/PlantAi                  （スタブ境界）
       -> actuator/PumpControl        （無効化されたスタブ）
       -> storage/PlantLog            （スタブ境界）

board/Board
  -> CommonFiles/Driver               クロック、ウォッチドッグ、Timer0
  -> CommonFiles/Power                POWER_KEEP、汎用入出力

ui/LcdUi
  -> drivers/Lcd
       -> drivers/LcdI2cf0
       -> CommonFiles/Timer            Timer1によるコマンド待機
```

ハードウェアレジスタへのアクセスは`board/`と`drivers/`に限定しています。
アプリケーション層は、それらのインターフェースだけに依存します。

## VS Codeでのビルドとデバッグ

標準の開発手順はリポジトリルートの
[`docs/VSCODE_SETUP.md`](../../../../docs/VSCODE_SETUP.md)にまとめています。
VS Codeでリポジトリルートを開き、`Ctrl+Shift+B`でDebugビルド、`F5`で
MCU-Linkへの書き込みとデバッグを実行できます。

CMakeビルドでも`ML63Q25x7`と`ML63Q2557`を定義し、このプロジェクト固有の
メモリ配置と64バイトの`.codeoption`を維持します。

## LEXIDE-Ω（移行期間の互換手順）

1. LEXIDEのCMSIS PackマネージャでARM CMSIS 5.9.0とROHM
   ML63Q25x7_DFP 1.1.0（または互換性のある新しいPack）をインストールする
2. **File > Import > General > Existing Projects into Workspace** を選択する
3. ルートディレクトリに`PlantDoctorWorkspace/PlantDoctor`を指定する
4. `PlantDoctor`プロジェクトが検出されることを確認する。CommonFilesへの
   相対リンクとリポジトリ配置を維持するため、**Copy projects into workspace**
   は選択せずにインポートを完了する
5. **Build Configurations > Set Active** で`Debug`または`Release`を選択する
6. 以前のデバッグセッションが動作中なら、赤い **Terminate** ボタンで終了する。
   OpenOCD/GDBが`PlantDoctor.elf`を開いたままにする場合があるため、
   ターゲットのSuspendだけでは不十分
7. **Project > Clean**、続いて **Project > Build Project** を実行する

両構成とも`ML63Q25x7`と`ML63Q2557`を定義し、リンカスクリプト
`ML63Q25x7_lccarm.ld`を使用します。CommonFilesは相対パスで参照し、
特定ユーザーの絶対パスをプロジェクト内に保存しません。

プロジェクト固有のリンカスクリプトは、通常のプログラムフラッシュ領域
`0x10000000..0x1003FFBF`の外側にある`0x1003FFC0`へ、ベンダー定義の
64バイト`.codeoption`セクションを保持します。これにより、リンク時の
ガベージコレクションでウォッチドッグのオプションワードが削除されることを防ぎます。

## MCU-Linkによる書き込み手順

1. DT-EBML63Q2557へUSB Type-Cから給電する
2. MCU-Linkを基板のSWD信号（SWDIO、SWCLK、GND、ターゲット基準電圧）へ
   接続する。基板がUSB給電されている場合は、MCU-Linkのターゲット電源出力を
   使用しない
3. MCU-LinkをUSBでPCへ接続する
4. **Run > Debug Configurations...** を開き、**LAPIS GDB Debugging (Arm)**
   構成を作成する。Mainタブで`PlantDoctor`プロジェクトと
   `Debug/PlantDoctor.elf`を選択する
5. DebuggerタブのICEに`CMSIS-DAP`を選択する。LEXIDEは`cmsis-dap.cfg`を
   使用し、選択中のML63Q25x7デバイスパックから`Cfg/ml63q25x7.cfg`が
   提供される。この設定によりSWDと256 KiBのフラッシュ配置が選択される
6. Startupタブでプロジェクト実行ファイルのロードと **Verify Flash Memory**
   を有効にして **Debug** を開始する。リセット、停止、ダウンロード、検証が
   完了した後、`main`から実行を再開する

プローブ名は、インストールしたLEXIDE/OpenOCDのリリースによって異なる場合が
あります。接続を試す前に、ターゲット電圧の表示を確認してください。

## 実機確認

1. 基板をリセットするか、電源を再投入する
2. LCDにタイトルと`BOARD TEST`が表示されることを確認する
3. LED1が1秒周期で点滅することを確認する。LED1/LED2/LED3の250 ms周期の
   交互点滅はエラー表示であり、通常のハートビートではない
4. SW1～SW4を個別に押し、対応する文字列がLCDに表示されることを確認する
5. 各スイッチを離し、表示が`BOARD TEST`へ戻ることを確認する
6. デバッガで停止し、`AppStateMachine.c`の`s_state`を確認する。正常動作時は
   `APP_STATE_MONITOR`かつ`PLANT_DOCTOR_ERROR_NONE`

LCDが表示されない場合は、最初にP4.6で5 Vレギュレータが有効になっていることと、
P7.5でバックライトが有効になっていることを確認します。続いてP7.2のリセット、
P7.3のSCLF0、P7.4のSDAF0、アドレス`0x7C`からのI2C ACK、状態機械の
エラーコードを確認します。LCDが表示されずエラーLEDパターンが見える場合は、
通常、LCD初期化、I2C配線、5 V電源のいずれかが失敗しています。

初回実機診断の内容と、修正版でもエラー状態になる場合にデバッガで確認する値は、
[`docs/HARDWARE_DIAGNOSTICS.md`](../../../../docs/HARDWARE_DIAGNOSTICS.md)を参照してください。
