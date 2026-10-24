/** =================================================================*
 * @file   PlantDoctorConfig.h
 * @brief  アプリケーション設定
 * ================================================================= */
#ifndef PLANT_DOCTOR_CONFIG_H
#define PLANT_DOCTOR_CONFIG_H

#define PLANT_DOCTOR_TICK_MS               (10U)
#define PLANT_DOCTOR_LED_BLINK_TICKS       (100U)
#define PLANT_DOCTOR_ERROR_BLINK_TICKS     (25U)
#define PLANT_DOCTOR_SELF_TEST_TICKS       (50U)
#define PLANT_DOCTOR_SENSOR_SAMPLE_TICKS   (100U)
#define PLANT_DOCTOR_SENSOR_DISPLAY_TICKS  (500U)
#define PLANT_DOCTOR_LCD_RECOVERY_INTERVAL_TICKS (100U)
#define PLANT_DOCTOR_LCD_RECOVERY_MAX_ATTEMPTS   (3U)
#define PLANT_DOCTOR_SWITCH_DEBOUNCE_POLLS (2U)
#define PLANT_DOCTOR_LCD_TIMEOUT_LOOPS     (50000UL)
#define PLANT_DOCTOR_I2C_TIMEOUT_LOOPS     (50000UL)
#define PLANT_DOCTOR_TIMER_START_TIMEOUT_LOOPS (480000UL)
#define PLANT_DOCTOR_PUMP_MAX_ON_TICKS     (1000U)   /* 1000 ticks = 10.0秒 (PP310-030 120ml/min -> 約20ml) */
#define PLANT_DOCTOR_PUMP_COOLDOWN_TICKS   (300U)
#define PLANT_DOCTOR_PUMP_MESSAGE_TICKS    (150U)
#define PLANT_DOCTOR_PUMP_REQUIRE_LIQUID   (true)
#define PLANT_DOCTOR_SOIL_CAL_DEFAULT_DRY  (1000U)
#define PLANT_DOCTOR_SOIL_CAL_DEFAULT_WET  (3000U)
#define PLANT_DOCTOR_SOIL_CAL_MIN_DIFF     (200U)
#define PLANT_DOCTOR_AIR_TEMP_MIN_CENTIC   (-4000)
#define PLANT_DOCTOR_AIR_TEMP_MAX_CENTIC   (8500)
#define PLANT_DOCTOR_LEAF_TEMP_MIN_CENTIC  (-7000)
#define PLANT_DOCTOR_LEAF_TEMP_MAX_CENTIC  (12000)
#define PLANT_DOCTOR_HUMIDITY_MAX_CENTIPC  (10000U)
#define PLANT_DOCTOR_TEMP_DELTA_MAX_CENTIC (2000)
#define PLANT_DOCTOR_PLAUSIBILITY_PERSIST  (5U)
#define PLANT_DOCTOR_SOIL_STEP_MAX_RAW     (1500U)

/* P4 診断しきい値（TODO(P2-7): 実測に基づく調整予定の暫定値） */
#define PLANT_DOCTOR_DIAG_DRY_THRESHOLD_PERMILLE     (300)     /* 土壌乾燥しきい値 (30.0%) */
#define PLANT_DOCTOR_DIAG_HEAT_DELTA_CENTIC          (200)     /* 熱ストレス葉温気温差 (+2.00℃、DEMO2) */
#define PLANT_DOCTOR_DIAG_HEAT_RATE_PER_HOUR         (50)      /* 熱ストレス葉温上昇速度 (+0.50℃/h) */
#define PLANT_DOCTOR_DIAG_LOW_LIGHT_ACCUM            (144000L) /* 日照不足積算照度しきい値 (100 Lux x 24h) */
#define PLANT_DOCTOR_DIAG_ROOT_UPTAKE_SOIL_PERMILLE  (400)     /* 吸水不良判定の土壌水分下限 (40.0%) */
#define PLANT_DOCTOR_DIAG_ROOT_UPTAKE_DELTA_CENTIC   (150)     /* 吸水不良葉温気温差 (+1.50℃) */
#define PLANT_DOCTOR_DIAG_SOIL_DRY_RATE_PER_HOUR     (-10)     /* 土壌乾燥傾向速度 (-10‰/h) */
#define PLANT_DOCTOR_DIAG_SOIL_WET_RATE_PER_HOUR     (10)      /* 土壌湿潤傾向速度 (+10‰/h) */

#define PLANT_DOCTOR_STRESS_BASE_SCORE               (0U)      /* 平常時ベーススコア (0〜100フルレンジ活用) */
#define PLANT_DOCTOR_STRESS_WEIGHT_SOIL              (35U)     /* 土壌水分の重み */
#define PLANT_DOCTOR_STRESS_WEIGHT_HEAT              (35U)     /* 熱ストレスの重み */
#define PLANT_DOCTOR_STRESS_WEIGHT_LIGHT             (15U)     /* 日照の重み */
#define PLANT_DOCTOR_STRESS_WEIGHT_HUMIDITY          (15U)     /* 湿度の重み */
#define PLANT_DOCTOR_STRESS_MAX_DOMINANCE_WEIGHT     (75U)     /* 最大値支配度[%] (最大寄り合成) */
#define PLANT_DOCTOR_STRESS_BASELINE_SOIL_PERMILLE   (600)     /* 平常土壌水分 [‰] */
#define PLANT_DOCTOR_STRESS_BASELINE_TEMP_DELTA      (80)      /* 平常葉温気温差 (+0.80℃、DEMO1) */
#define PLANT_DOCTOR_STRESS_MAX_HEAT_DELTA_RANGE     (167)     /* 熱ストレスフルスケール差分 (+1.67℃) */
#define PLANT_DOCTOR_STRESS_BASELINE_LIGHT_ACCUM     (144000L) /* 平常積算照度下限 (100 Lux x 24h) */
#define PLANT_DOCTOR_STRESS_BASELINE_HUMIDITY        (6000U)   /* 平常湿度 (60.00%RH) */

/* P5-5 自律水やり設定（P5-1/P5-2未検証のため既定無効。TODO(P2-7): 暫定値） */
#define PLANT_DOCTOR_WATERING_AUTO_ENABLE            (true)    /* 自律水やりの有効/無効 (ゲート2確認済: true) */
#define PLANT_DOCTOR_WATERING_DRY_THRESHOLD_PERMILLE (300)     /* 給水開始乾燥しきい値 [‰] */
#define PLANT_DOCTOR_WATERING_MIN_INTERVAL_SECONDS   (1800UL)  /* 最小給水間隔 [秒] (30分) */
#define PLANT_DOCTOR_WATERING_MIN_INTERVAL_TICKS     (180000UL)/* 最小給水間隔 [ticks] (1800s * 100) */
#define PLANT_DOCTOR_WATERING_MAX_DAILY_COUNT        (6U)      /* 1日あたり最大給水回数 */

/* P6-1 給水応答自己診断設定（TODO(P2-7): 実測に基づく調整予定の暫定値） */
#define PLANT_DOCTOR_RESP_IMMEDIATE_SECONDS          (30U)     /* 給水直後測定時間 [秒] */
#define PLANT_DOCTOR_RESP_DELAYED_SECONDS            (180U)    /* 浸透後測定時間 [秒] */
#define PLANT_DOCTOR_RESP_TEMP_DELTA_SECONDS         (300U)    /* 蒸散回復測定時間 [秒] */
#define PLANT_DOCTOR_RESP_TIMEOUT_SECONDS            (600U)    /* 測定上限タイムアウト [秒] */
#define PLANT_DOCTOR_RESP_MIN_SOIL_RECOVERY_PERMILLE (50)      /* 回復判定最小土壌水分増加量 [‰] */
#define PLANT_DOCTOR_RESP_MIN_TEMP_DROP_CENTIC       (30)      /* 葉温回復判定最小低下量 [1/100 ℃] */
#define PLANT_DOCTOR_RESP_FAILED_HOLD_SECONDS        (1800UL)  /* 給水失敗を保つ時間 [秒]。過ぎたら解除し自律給水を再開できる */

/* P6-2 土壌劣化推定設定（TODO(P2-7): 実測に基づく調整予定の暫定値） */
#define PLANT_DOCTOR_DEGRADE_REQUIRED_CYCLES         (3U)      /* 劣化判定に必要な連続サイクル数 */
#define PLANT_DOCTOR_DEGRADE_MIN_GAIN_PERMILLE       (80)      /* 保水性低下判定の増加量下限 [‰] */
#define PLANT_DOCTOR_DEGRADE_FAST_DRY_RATE           (-25)     /* 異常急速低下速度 [‰/h] */

/* Solist-AI による「いつもと違う」判定（再構成誤差の、その株の平常値からのずれ）
 * TODO: 実機で誤差の水準を見て調整する暫定値。サンプルは1秒ごと */
#define PLANT_DOCTOR_AI_ANOMALY_ENABLE                (true)    /* 判定の有効/無効 */
#define PLANT_DOCTOR_AI_ANOMALY_MIN_TRAIN_COUNT       (100U)    /* 基準を作り始める学習回数 */
#define PLANT_DOCTOR_AI_ANOMALY_BASELINE_SHIFT        (10U)     /* 基準の移動平均 1/1024（約17分） */
#define PLANT_DOCTOR_AI_ANOMALY_MIN_BASELINE_SAMPLES  (600UL)   /* 判定開始までの健康サンプル数（約10分） */
#define PLANT_DOCTOR_AI_ANOMALY_MIN_DEVIATION_PPM     (15U)     /* ばらつきの下限 [ppm] */
#define PLANT_DOCTOR_AI_ANOMALY_DEVIATION_SCALE       (8U)      /* 異常度100 = ばらつきの8倍 */
#define PLANT_DOCTOR_AI_ANOMALY_ON_SCORE              (50U)     /* 「いつもと違う」の開始スコア */
#define PLANT_DOCTOR_AI_ANOMALY_OFF_SCORE             (30U)     /* 解除スコア */
#define PLANT_DOCTOR_AI_ANOMALY_ON_SAMPLES            (60U)     /* 約1分続いたら判定 */
#define PLANT_DOCTOR_AI_ANOMALY_OFF_SAMPLES           (120U)    /* 約2分続いたら解除 */
#define PLANT_DOCTOR_AI_ANOMALY_RELEARN_EVERY_SAMPLES (60U)     /* いつもと違う間は約1分に1回だけ学ぶ */
/* 学び直しが早すぎると、ゆっくり進む乾燥まで「いつも」として覚えてしまう（想定値の計算で確認） */
#define PLANT_DOCTOR_AI_ANOMALY_RELEARN_AFTER_SAMPLES (10800U)  /* 約3時間続いたら新しいいつもとして学び直す */
/* 変化速度は直近60分の履歴で計算する。給水後は土が湿り終わってから履歴が入れ替わるまで、起動直後は履歴がたまるまで、
 * 変化速度をAIの判定に使わない */
#define PLANT_DOCTOR_AI_POST_WATERING_HOLD_SAMPLES    (5400U)   /* 給水後にAIの判定と学習を休むサンプル数（約1時間半） */
#define PLANT_DOCTOR_AI_RATE_READY_MINUTES            (15U)     /* 変化速度をAIへ入れ始める履歴の長さ [分] */
#define PLANT_DOCTOR_AI_LEAF_RATE_RANGE_CENTIC        (300)     /* 葉温の変化速度の正規化範囲 ±3.00℃/h */
#define PLANT_DOCTOR_AI_SOIL_RATE_RANGE_PERMILLE      (60)      /* 土壌水分の変化速度の正規化範囲 ±60‰/h */

/* P8-1 デモモード設定 */
#define PLANT_DOCTOR_DEMO_MODE_ENABLE                (true)    /* デモモード機能有効フラグ */
#define PLANT_DOCTOR_DEMO_SW1_HOLD_TICKS             (300U)    /* SW1長押し判定時間 [ticks] (3.0秒) */

/* P2-4 UARTコンソール設定 */
#define PLANT_DOCTOR_CONSOLE_BAUDRATE                (115200UL)/* ボーレート [bps] */
#define PLANT_DOCTOR_CONSOLE_BR_DIVISOR              (26U)     /* 48MHz / (16 * 115200) = 26 (0.16%誤差) */
#define PLANT_DOCTOR_CONSOLE_MAX_BYTES_PER_TICK      (16U)     /* 1 tickあたりの最大送受信バイト数 */
#define PLANT_DOCTOR_CONSOLE_LINE_MAX                (32U)     /* コマンド1行の最大長 */
#define PLANT_DOCTOR_CONSOLE_TX_BUFFER_SIZE          (256U)    /* 送信リングバッファサイズ */
#define PLANT_DOCTOR_CONSOLE_RX_BUFFER_SIZE          (64U)     /* 受信リングバッファサイズ */

/* P2-6 ログ記録キュー設定 */
#define PLANT_DOCTOR_LOG_PERIODIC_INTERVAL_TICKS     (6000U)   /* 定期記録周期 [ticks] (60秒) */
#define PLANT_DOCTOR_LOG_QUEUE_SIZE                  (8U)      /* ログ待機キューサイズ */

#endif /* PLANT_DOCTOR_CONFIG_H */


