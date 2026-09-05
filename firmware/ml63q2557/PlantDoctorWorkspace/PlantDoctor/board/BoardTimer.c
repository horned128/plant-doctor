/** =================================================================*
 * @file   BoardTimer.c
 * @brief  基板周期タイマ
 * ================================================================= */
#include "BoardTimer.h"                                     /* BoardTimerのAPIと型定義 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorConfig.h"                              /* PlantDoctorConfigのAPIと型定義 */
#include "irq.h"                                            /* 割り込み制御API */
#include "smpl_common.h"                                    /* 共通周辺機器制御API */
#include "timer0_1.h"                                       /* タイマ0 API */

#define BOARD_TIMER_LSCLK_HZ               (32768UL)
#define BOARD_TIMER_COUNT_VALUE            (((BOARD_TIMER_LSCLK_HZ * PLANT_DOCTOR_TICK_MS) / 1000UL) - 1UL)
#define BOARD_TIMER_MODE                   (TM_CS_LSCLK | TM_DIV1 | TM_MODE_16BIT | TM_OST_RELOAD)

static volatile uint8_t s_pendingTicks;                     /**< モジュール内部状態 */
static volatile bool s_tickOverflow;                        /**< モジュール内部状態 */

/** =================================================================*
 * @brief  BoardTimer_WaitUntilRunning処理
 * @return 実行結果または取得値
 * ================================================================= */
static bool BoardTimer_WaitUntilRunning(void) {
    uint32_t remaining = PLANT_DOCTOR_TIMER_START_TIMEOUT_LOOPS;

    /* TMSTATはLSCLKと同期するため、状態変化には遅延がある。 */
    while ((timer0_getStatus() == 0U) && (remaining > 0UL)) {
        --remaining;
    }

    return (timer0_getStatus() != 0U);
}

/** =================================================================*
 * @brief  BoardTimer_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool BoardTimer_Init(void) {
    uint32_t interruptState = __get_PRIMASK();

    __disable_irq();
    smpl_enablePeripheral(TM0_PERI);
    irq_tm0_dis();
    irq_tm0_clearIRQ();
    timer0_init(BOARD_TIMER_MODE);
    timer0_setCnt((uint16_t)BOARD_TIMER_COUNT_VALUE);
    s_pendingTicks = 0U;
    s_tickOverflow = false;
    irq_tm0_setLevel(1U);
    irq_tm0_ena();
    timer0_start();
    if (interruptState == 0U) {
        __enable_irq();
    }

    return BoardTimer_WaitUntilRunning();
}

/** =================================================================*
 * @brief  BoardTimer_Take10MsTick処理
 * @return 実行結果または取得値
 * ================================================================= */
bool BoardTimer_Take10MsTick(void) {
    bool available = false;
    uint32_t interruptState = __get_PRIMASK();

    __disable_irq();
    if (s_pendingTicks > 0U) {
        --s_pendingTicks;
        available = true;
    }
    if (interruptState == 0U) {
        __enable_irq();
    }

    return available;
}

/** =================================================================*
 * @brief  BoardTimer_TakeOverflow処理
 * @return 実行結果または取得値
 * ================================================================= */
bool BoardTimer_TakeOverflow(void) {
    bool overflow;
    uint32_t interruptState = __get_PRIMASK();

    __disable_irq();
    overflow = s_tickOverflow;
    s_tickOverflow = false;
    if (interruptState == 0U) {
        __enable_irq();
    }

    return overflow;
}

/** =================================================================*
 * @brief  TM0_IRQHandler処理
 * ================================================================= */
void TM0_IRQHandler(void) {
    if (s_pendingTicks < UINT8_MAX) {
        ++s_pendingTicks;
    } else {
        s_tickOverflow = true;
    }
}
