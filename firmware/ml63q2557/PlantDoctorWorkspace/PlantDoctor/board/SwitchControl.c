/** =================================================================*
 * @file   SwitchControl.c
 * @brief  スイッチ制御
 * ================================================================= */
#include "SwitchControl.h"                                  /* SwitchControlのAPIと型定義 */
#include "Input.h"                                          /* 入力デバイスAPI */
#include "PlantDoctorConfig.h"                              /* PlantDoctorConfigのAPIと型定義 */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "rdwr_reg.h"                                       /* レジスタ操作API */

#define SWITCH_PULLUP_VALUE                (0xFFU)
#define SWITCH_PUSH_MASK                   (0x0FU)
#define SWITCH_DIP_MASK                    (0xF0U)
#define SWITCH_DSW_CONFIG                  ((0x01UL << 24U) | (0x01UL << 16U) | (0x01UL << 8U) | (0x01UL << 0U))
#define SWITCH_PSW_CONFIG                  ((0x01UL << 24U) | (0x01UL << 16U) | (0x01UL << 8U) | (0x01UL << 0U))

static uint8_t s_pressedMask;                               /**< モジュール内部状態 */

/** =================================================================*
 * @brief  SwitchControl_ReadRaw処理
 * @return 実行結果または取得値
 * ================================================================= */
static uint8_t SwitchControl_ReadRaw(void) {
    uint8_t push = (uint8_t)(PORT5->P5DI & SWITCH_PUSH_MASK);
    uint8_t dip = (uint8_t)(PORT3->P3DI & SWITCH_DIP_MASK);
    return (uint8_t)(push | dip);
}

/** =================================================================*
 * @brief  SwitchControl_UpdateState処理
 * ================================================================= */
static void SwitchControl_UpdateState(void) {
    uint8_t stableValue = InputGetSystemInputValue(INPUT_INDEX_DIP_PSH);
    s_pressedMask = (uint8_t)((~stableValue) & SWITCH_PUSH_MASK);
}

/** =================================================================*
 * @brief  SwitchControl_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool SwitchControl_Init(void) {
    set_reg32(PORT3->P3MOD1, SWITCH_DSW_CONFIG);
    set_reg32(PORT5->P5MOD0, SWITCH_PSW_CONFIG);
    s_pressedMask = 0U;
    return InputInit(INPUT_INDEX_DIP_PSH, SWITCH_PULLUP_VALUE);
}

/** =================================================================*
 * @brief  SwitchControl_Process10Ms処理
 * @return 実行結果または取得値
 * ================================================================= */
bool SwitchControl_Process10Ms(void) {
    INPUT_POLLING_RESULT result = InputPolling(INPUT_INDEX_DIP_PSH,
        (uint8_t)PLANT_DOCTOR_SWITCH_DEBOUNCE_POLLS, SwitchControl_ReadRaw, SwitchControl_UpdateState);

    return (result != INPUT_POLLING_RESULT_INVALID_ARGUMENT);
}

/** =================================================================*
 * @brief  SwitchControl_GetPressedMask処理
 * @return 実行結果または取得値
 * ================================================================= */
uint8_t SwitchControl_GetPressedMask(void) {
    return s_pressedMask;
}
