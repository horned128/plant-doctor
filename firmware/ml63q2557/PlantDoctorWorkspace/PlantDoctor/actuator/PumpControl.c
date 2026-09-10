/** =================================================================*
 * @file   PumpControl.c
 * @brief  ポンプ制御
 * ================================================================= */
#include "PumpControl.h"                                    /* PumpControlのAPIと型定義 */
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
    return true;
}

/** =================================================================*
 * @brief  緊急時にポンプ出力を即座に遮断する。
 * ================================================================= */
void PumpControl_EmergencyStop(void) {
    set_bit(PORT6->P6DO, PUMP_PORT_MASK);
    s_isOn = false;
    s_onTicks = 0U;
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
        return PUMP_CONTROL_STATUS_OK;
    }

    if (s_isOn) {
        /* SSR (OUT0) を開放するためHighを出力する */
        set_bit(PORT6->P6DO, PUMP_PORT_MASK);
        s_isOn = false;
        s_onTicks = 0U;
        s_cooldownTicks = PLANT_DOCTOR_PUMP_COOLDOWN_TICKS;
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
            (void)PumpControl_Request(false);
        }
    } else if (s_cooldownTicks > 0U) {
        --s_cooldownTicks;
    }
}

/** =================================================================*
 * @brief  PumpControl_IsOn処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PumpControl_IsOn(void) {
    return s_isOn;
}
