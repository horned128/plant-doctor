/** =================================================================*
 * @file   PumpControl.c
 * @brief  ポンプ制御
 * ================================================================= */
#include "PumpControl.h"                                    /* PumpControlのAPIと型定義 */

static bool s_isOn;                                         /**< モジュール内部状態 */

/** =================================================================*
 * @brief  PumpControl_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PumpControl_Init(void) {
    s_isOn = false;
    return true;
}

/** =================================================================*
 * @brief  PumpControl_Request処理
 * @param[in] on 引数
 * @return 実行結果または取得値
 * ================================================================= */
PUMP_CONTROL_STATUS PumpControl_Request(bool on) {
    (void)on;
    s_isOn = false;
    return PUMP_CONTROL_STATUS_NOT_IMPLEMENTED;
}

/** =================================================================*
 * @brief  PumpControl_IsOn処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PumpControl_IsOn(void) {
    return s_isOn;
}
