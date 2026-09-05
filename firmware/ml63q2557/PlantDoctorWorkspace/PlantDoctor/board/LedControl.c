/** =================================================================*
 * @file   LedControl.c
 * @brief  LED制御
 * ================================================================= */
#include "LedControl.h"                                     /* LedControlのAPIと型定義 */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "rdwr_reg.h"                                       /* レジスタ操作API */

#define LED_PORT_MASK                      ((1UL << 4U) | (1UL << 5U) | (1UL << 6U))
#define LED1_MODE_MASK                     (0x3FUL << 0U)
#define LED2_MODE_MASK                     (0x3FUL << 8U)
#define LED3_MODE_MASK                     (0x3FUL << 16U)
#define LED1_OUTPUT_MODE                   (0x02UL << 0U)
#define LED2_OUTPUT_MODE                   (0x02UL << 8U)
#define LED3_OUTPUT_MODE                   (0x02UL << 16U)

static bool s_ledState[LED_CONTROL_COUNT];                  /**< モジュール内部状態 */

/** =================================================================*
 * @brief  LedControl_GetMask処理
 * @param[in] id 引数
 * @return 実行結果または取得値
 * ================================================================= */
static uint32_t LedControl_GetMask(LED_CONTROL_ID id) {
    return (1UL << (4U + (uint32_t)id));
}

/** =================================================================*
 * @brief  LedControl_Init処理
 * ================================================================= */
void LedControl_Init(void) {
    clear_bit(PORT5->P5DO, LED_PORT_MASK);
    write_bit(PORT5->P5MOD1, LED1_MODE_MASK, LED1_OUTPUT_MODE);
    write_bit(PORT5->P5MOD1, LED2_MODE_MASK, LED2_OUTPUT_MODE);
    write_bit(PORT5->P5MOD1, LED3_MODE_MASK, LED3_OUTPUT_MODE);
    s_ledState[LED_CONTROL_1] = false;
    s_ledState[LED_CONTROL_2] = false;
    s_ledState[LED_CONTROL_3] = false;
}

/** =================================================================*
 * @brief  LedControl_Set処理
 * @param[in] id 引数
 * @param[in] on 引数
 * ================================================================= */
void LedControl_Set(LED_CONTROL_ID id, bool on) {
    uint32_t mask;

    if (id >= LED_CONTROL_COUNT) {
        return;
    }

    mask = LedControl_GetMask(id);
    if (on) {
        set_bit(PORT5->P5DO, mask);
    } else {
        clear_bit(PORT5->P5DO, mask);
    }
    s_ledState[id] = on;
}

/** =================================================================*
 * @brief  LedControl_Toggle処理
 * @param[in] id 引数
 * ================================================================= */
void LedControl_Toggle(LED_CONTROL_ID id) {
    if (id < LED_CONTROL_COUNT) {
        LedControl_Set(id, !s_ledState[id]);
    }
}

/** =================================================================*
 * @brief  LedControl_AllOff処理
 * ================================================================= */
void LedControl_AllOff(void) {
    LedControl_Set(LED_CONTROL_1, false);
    LedControl_Set(LED_CONTROL_2, false);
    LedControl_Set(LED_CONTROL_3, false);
}
