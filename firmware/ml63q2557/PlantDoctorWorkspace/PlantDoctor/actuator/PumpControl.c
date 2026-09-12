/** =================================================================*
 * @file   PumpControl.c
 * @brief  ポンプ制御
 * ================================================================= */
#include "PumpControl.h"                                    /* PumpControlのAPIと型定義 */
#include <stddef.h>                                         /* NULL定義 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorConfig.h"                              /* アプリケーション設定 */
#include "TankLevelSensor.h"                                /* タンク液面センサーAPI */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "rdwr_reg.h"                                       /* レジスタ操作API */

#define PUMP_PORT_MASK                     (1UL << 6U)
#define PUMP_MODE_MASK                     (0x3FUL << 16U)
#define PUMP_OUTPUT_MODE                   (0x0AUL << 16U)

static bool s_isOn;                                         /**< モジュール内部状態 */
static uint16_t s_onTicks;                                  /**< 連続駆動Tickカウンタ */
static uint16_t s_cooldownTicks;                            /**< クールダウンTickカウンタ */

static PUMP_WATERING_EVENT s_preContext;                    /**< 給水直前の文脈情報 */
static PUMP_WATERING_EVENT s_currentEvent;                  /**< 進行中給水イベント */
static PUMP_WATERING_EVENT s_lastCompletedEvent;            /**< 確定した最新給水イベント */
static bool s_eventPending;                                 /**< 未取得イベント有無フラグ */

/** =================================================================*
 * @brief  ポンプ停止の内部共通処理
 * @param[in] reason 停止理由
 * ================================================================= */
static void PumpControl_StopInternal(PUMP_STOP_REASON reason) {
    if (s_isOn) {
        /* SSR (OUT0) を開放するためHighを出力する */
        set_bit(PORT6->P6DO, PUMP_PORT_MASK);
        s_isOn = false;
        s_currentEvent.onDurationTicks = s_onTicks;
        s_currentEvent.stopReason = (uint8_t)reason;
        s_lastCompletedEvent = s_currentEvent;
        s_eventPending = true;
        s_onTicks = 0U;
        if (reason != PUMP_STOP_REASON_EMERGENCY) {
            s_cooldownTicks = PLANT_DOCTOR_PUMP_COOLDOWN_TICKS;
        }
    }
}

/** =================================================================*
 * @brief  PumpControl_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PumpControl_Init(void) {
    /* 起動時の突入・誤動作を防ぐため、出力設定前に確実にOFF (High) とする */
    set_bit(PORT6->P6DO, PUMP_PORT_MASK);
    write_bit(PORT6->P6MOD1, PUMP_MODE_MASK, PUMP_OUTPUT_MODE);
    s_isOn = false;
    s_onTicks = 0U;
    s_cooldownTicks = 0U;
    s_eventPending = false;
    s_preContext.startTimeSeconds = 0xFFFFFFFFU;
    s_preContext.onDurationTicks = 0U;
    s_preContext.soilMoistureRawBefore = 0U;
    s_preContext.soilMoisturePermilleBefore = 0;
    s_preContext.leafAirDeltaBefore = 0;
    s_preContext.stopReason = (uint8_t)PUMP_STOP_REASON_MANUAL;
    s_preContext.tankLiquidAtStart = false;
    s_preContext.automatic = false;
    return true;
}

/** =================================================================*
 * @brief  給水開始前の文脈情報を設定する。
 * @param[in] timeSeconds    給水開始時のUNIX秒
 * @param[in] soilRaw        給水直前の土壌水分生値
 * @param[in] soilPermille   給水直前の土壌水分千分率
 * @param[in] leafAirDelta   給水直前の葉温気温差
 * @param[in] automatic      自律給水フラグ
 * ================================================================= */
void PumpControl_SetPreWateringContext(uint32_t timeSeconds,
                                       uint16_t soilRaw,
                                       int16_t soilPermille,
                                       int16_t leafAirDelta,
                                       bool automatic) {
    s_preContext.startTimeSeconds = timeSeconds;
    s_preContext.soilMoistureRawBefore = soilRaw;
    s_preContext.soilMoisturePermilleBefore = soilPermille;
    s_preContext.leafAirDeltaBefore = leafAirDelta;
    s_preContext.automatic = automatic;
}

/** =================================================================*
 * @brief  緊急時にポンプ出力を即座に遮断する。
 * ================================================================= */
void PumpControl_EmergencyStop(void) {
    PumpControl_StopInternal(PUMP_STOP_REASON_EMERGENCY);
}

/** =================================================================*
 * @brief  PumpControl_Request処理
 * @param[in] on 引数
 * @return 実行結果または取得値
 * ================================================================= */
PUMP_CONTROL_STATUS PumpControl_Request(bool on) {
    if (on) {
        if (s_isOn) {
            return PUMP_CONTROL_STATUS_OK;
        }
        if (PLANT_DOCTOR_PUMP_REQUIRE_LIQUID && !TankLevelSensor_IsLiquidDetected()) {
            return PUMP_CONTROL_STATUS_EMPTY;
        }
        if (s_cooldownTicks > 0U) {
            return PUMP_CONTROL_STATUS_COOLDOWN;
        }
        /* SSR (OUT0) を導通させるためLowを出力する */
        clear_bit(PORT6->P6DO, PUMP_PORT_MASK);
        s_isOn = true;
        s_onTicks = 0U;

        s_currentEvent.startTimeSeconds = s_preContext.startTimeSeconds;
        s_currentEvent.soilMoistureRawBefore = s_preContext.soilMoistureRawBefore;
        s_currentEvent.soilMoisturePermilleBefore = s_preContext.soilMoisturePermilleBefore;
        s_currentEvent.leafAirDeltaBefore = s_preContext.leafAirDeltaBefore;
        s_currentEvent.tankLiquidAtStart = TankLevelSensor_IsLiquidDetected();
        s_currentEvent.automatic = s_preContext.automatic;

        /* 次回のために文脈を既定値へ戻す */
        s_preContext.startTimeSeconds = 0xFFFFFFFFU;
        s_preContext.soilMoistureRawBefore = 0U;
        s_preContext.soilMoisturePermilleBefore = 0;
        s_preContext.leafAirDeltaBefore = 0;
        s_preContext.automatic = false;

        return PUMP_CONTROL_STATUS_OK;
    }

    if (s_isOn) {
        PumpControl_StopInternal(PUMP_STOP_REASON_MANUAL);
    }
    return PUMP_CONTROL_STATUS_OK;
}

/** =================================================================*
 * @brief  10ms周期でタイマ減算と自動停止を管理する。
 * ================================================================= */
void PumpControl_Process10Ms(void) {
    if (s_isOn) {
        if (s_onTicks < UINT16_MAX) {
            ++s_onTicks;
        }
        if (s_onTicks >= PLANT_DOCTOR_PUMP_MAX_ON_TICKS) {
            PumpControl_StopInternal(PUMP_STOP_REASON_MAX_ON_TIME);
        }
    } else if (s_cooldownTicks > 0U) {
        --s_cooldownTicks;
    }
}

/** =================================================================*
 * @brief  未取得の給水イベントを取り出す。
 * @param[out] event イベント出力先
 * @return 新規イベントが存在した場合はtrue
 * ================================================================= */
bool PumpControl_TakeWateringEvent(PUMP_WATERING_EVENT *event) {
    if ((event == NULL) || (!s_eventPending)) {
        return false;
    }
    *event = s_lastCompletedEvent;
    s_eventPending = false;
    return true;
}

/** =================================================================*
 * @brief  PumpControl_IsOn処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PumpControl_IsOn(void) {
    return s_isOn;
}
