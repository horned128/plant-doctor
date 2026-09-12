/** =================================================================*
 * @file   Board.c
 * @brief  基板初期化と周期処理
 * ================================================================= */
#include "Board.h"                                          /* BoardのAPIと型定義 */
#include "BoardTimer.h"                                     /* BoardTimerのAPIと型定義 */
#include "I2cBus.h"                                         /* I2cBusのAPIと型定義 */
#include "LedControl.h"                                     /* LedControlのAPIと型定義 */
#include "PowerControlAdapter.h"                            /* PowerControlAdapterのAPIと型定義 */
#include "SwitchControl.h"                                  /* SwitchControlのAPIと型定義 */
#include "clock.h"                                          /* クロック制御API */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "smpl_common.h"                                    /* 共通周辺機器制御API */
#include "wdt.h"                                            /* ウォッチドッグAPI */
#include "SoftSpi.h"                                        /* ソフトウェアSPI制御API */
#include "FramDriver.h"                                     /* FeRAMドライバAPI */

/** =================================================================*
 * @brief  Board_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
PLANT_DOCTOR_ERROR Board_Init(void) {
    PLANT_DOCTOR_ERROR error = PLANT_DOCTOR_ERROR_NONE;

    __disable_irq();
    wdt_init(WDT_2S);
    wdt_clear();
    smpl_setLsCrystal32Khz();
    smpl_setHsPll48Mhz(CLK_XSPEN_DIS, CLK_HXSPEN_DIS);

    if (!PowerControlAdapter_Init()) {
        error = PLANT_DOCTOR_ERROR_POWER;
    }
    SoftSpiPeripheralInit();
    FramDriver_Init();
    LedControl_Init();
    if (!SwitchControl_Init() && (error == PLANT_DOCTOR_ERROR_NONE)) {
        error = PLANT_DOCTOR_ERROR_SWITCH;
    }
    (void)I2cBus_Init();
    if (!BoardTimer_Init() && (error == PLANT_DOCTOR_ERROR_NONE)) {
        error = PLANT_DOCTOR_ERROR_TIMER;
    }
    __enable_irq();

    return error;
}

/** =================================================================*
 * @brief  Board_Process10Ms処理
 * @return 実行結果または取得値
 * ================================================================= */
bool Board_Process10Ms(void) {
    return SwitchControl_Process10Ms();
}

/** =================================================================*
 * @brief  Board_Take10MsTick処理
 * @return 実行結果または取得値
 * ================================================================= */
bool Board_Take10MsTick(void) {
    return BoardTimer_Take10MsTick();
}

/** =================================================================*
 * @brief  Board_TakeTickOverflow処理
 * @return 実行結果または取得値
 * ================================================================= */
bool Board_TakeTickOverflow(void) {
    return BoardTimer_TakeOverflow();
}

/** =================================================================*
 * @brief  Board_GetPressedSwitchMask処理
 * @return 実行結果または取得値
 * ================================================================= */
uint8_t Board_GetPressedSwitchMask(void) {
    return SwitchControl_GetPressedMask();
}

/** =================================================================*
 * @brief  Board_IsPowerHeld処理
 * @return 実行結果または取得値
 * ================================================================= */
bool Board_IsPowerHeld(void) {
    return PowerControlAdapter_IsPowerHeld();
}

/** =================================================================*
 * @brief  Board_ServiceWatchdog処理
 * ================================================================= */
void Board_ServiceWatchdog(void) {
    wdt_clear();
}

/** =================================================================*
 * @brief  Board_GetMaxPendingTicks処理
 * @return 実行結果または取得値
 * ================================================================= */
uint8_t Board_GetMaxPendingTicks(void) {
    return BoardTimer_GetMaxPendingTicks();
}

/** =================================================================*
 * @brief  Board_ClearMaxPendingTicks処理
 * ================================================================= */
void Board_ClearMaxPendingTicks(void) {
    BoardTimer_ClearMaxPendingTicks();
}

