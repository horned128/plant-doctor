/** =================================================================*
 * @file   PowerControlAdapter.c
 * @brief  電源制御アダプタ
 * ================================================================= */
#include "PowerControlAdapter.h"                            /* PowerControlAdapterのAPIと型定義 */
#include "Output.h"                                         /* 出力デバイスAPI */
#include "SystemPowerControl.h"                             /* 基板電源制御API */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "rdwr_reg.h"                                       /* レジスタ操作API */

#define POWER_KEEP_MASK                    (1UL << 5U)
#define REGULATOR_5V_MASK                  (1UL << 6U)
#define REGULATOR_5V_MODE                  (0x02UL << 16U)

/** =================================================================*
 * @brief  PowerControlAdapter_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PowerControlAdapter_Init(void) {
    SystemPowerControlInit();

    /* LCDバックライトに基板の5 Vレギュレータを使用する。 */
    set_bit(PORT4->P4MOD1, REGULATOR_5V_MODE);
    (void)OutputOnUInt32(&(PORT4->P4DO), REGULATOR_5V_MASK);

    return PowerControlAdapter_IsPowerHeld() && get_bit(PORT4->P4DO, REGULATOR_5V_MASK);
}

/** =================================================================*
 * @brief  PowerControlAdapter_IsPowerHeld処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PowerControlAdapter_IsPowerHeld(void) {
    return get_bit(PORT4->P4DO, POWER_KEEP_MASK);
}

/** =================================================================*
 * @brief  PowerControlAdapter_Shutdown処理
 * ================================================================= */
void PowerControlAdapter_Shutdown(void) {
    (void)OutputOffUInt32(&(PORT4->P4DO), REGULATOR_5V_MASK);
    SystemPowerControlFin();
}
