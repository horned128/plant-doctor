# 実機診断

## 2026-07-20 初回実機確認

元のファームウェアはLEXIDEとMCU-Linkを使って正常にビルド、書き込み、起動
できました。LEDは変化しましたが、LCDには何も表示されず、スイッチを押しても
目に見える反応がありませんでした。

動作中のOpenOCDセッションを通して読み出しのみの調査を行い、次を確認しました。

- `s_state == APP_STATE_ERROR` (`3`).
- `s_error == PLANT_DOCTOR_ERROR_TIMER` (`2`).
- Timer0はカウント中で、起動後の`TMSTAT == 1`だった
- LCD関連のP7レジスタとI2CF0レジスタはゼロのままで、LCD初期化に到達していなかった
- P3/P5のスイッチ入力は設定済みでHighを読み出していたが、アプリケーションが
  `APP_STATE_ERROR`の間は、仕様によりスイッチのフィードバック処理が動作しない

確認されたLED動作は250 ms周期の交互エラー表示で、通常時のLED1による
1秒周期のハートビートではありませんでした。

## 原因と修正

修正前の`BoardTimer_Init()`は、`timer0_start()`の直後に
`timer0_getStatus()`を読み出していました。タイマ状態はLSCLKに同期するため、
タイマが正常に起動していても、最初の読み出しではゼロのままになる場合があります。
この過渡的な値を`PLANT_DOCTOR_ERROR_TIMER`として報告したため、状態機械が
LCD初期化とスイッチ監視まで到達できませんでした。

`BoardTimer_Init()`は、上限付きの`PLANT_DOCTOR_TIMER_START_TIMEOUT_LOOPS`を
使って動作状態を待つように修正しました。割り込みハンドラは引き続きTick状態の
更新だけを行います。LCD、スイッチ、その他のペリフェラル処理を割り込み
コンテキストへ移していません。

## 再確認手順

1. LEXIDEの赤い **Terminate** ボタンで動作中のデバッグセッションを終了する
2. **Project > Clean**、続いて **Project > Build Project** を実行する
3. `Debug/PlantDoctor.elf`の更新日時が新しくなっていることを確認する
4. PlantDoctorのデバッグ構成を起動し、フラッシュへの書き込みと検証を行う
5. 実行を再開し、必要に応じてターゲットをリセットまたは電源再投入する
6. `PLANT DOCTOR`／`BOARD TEST`、LED1の1秒周期ハートビート、SW1～SW4の
   フィードバックを確認する

修正版でも失敗する場合は、ターゲットを停止し、`AppStateMachine.c`の
グローバル変数`s_state`と`s_error`を確認してください。関連するエラー値は
次のとおりです。

| 値 | 意味 |
|---:|---|
| 0 | `PLANT_DOCTOR_ERROR_NONE` |
| 1 | `PLANT_DOCTOR_ERROR_POWER` |
| 2 | `PLANT_DOCTOR_ERROR_TIMER` |
| 3 | `PLANT_DOCTOR_ERROR_SWITCH` |
| 4 | `PLANT_DOCTOR_ERROR_LCD_INIT` |
| 5 | `PLANT_DOCTOR_ERROR_LCD_IO` |

LCDエラーの場合は、P4.6（5 V有効化）、P7.5（バックライト）、P7.2（LCDリセット）、
P7.3/P7.4（SCLF0/SDAF0）、I2CF0のACK状態を確認してください。ペリフェラル
ドライバを変更する前に、`s_state`、`s_error`、観測したLEDパターンを記録します。
