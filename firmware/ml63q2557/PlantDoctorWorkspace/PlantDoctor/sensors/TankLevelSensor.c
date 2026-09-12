/** =================================================================*
 * @file   TankLevelSensor.c
 * @brief  液面センサー
 * ================================================================= */
#include "TankLevelSensor.h"                                /* TankLevelSensorのAPIと型定義 */
#include "mcu.h"                                            /* ML63Q2557のレジスタ定義 */
#include "rdwr_reg.h"                                       /* レジスタ操作API */

#define SEN0204_INPUT_MASK                 (1UL << 4U)
#define PHOTO_COUPLER_INPUT_CONFIG         ((0x01UL << 8U) | (0x01UL << 0U))

/** =================================================================*
 * @brief  SEN0204用のCN5 IN0入力を設定する。
 * @note   P6MOD1のP64 (bit 0-3) およびP65 (bit 8-11) を設定する。
 *         PumpControl_Init() が設定するP66 (bit 16-21) とは独立しており、
 *         App_Init() における初期化順序に依存しない。
 * @return 初期化成功時はtrue
 * ================================================================= */
bool TankLevelSensor_Init(void) {
    write_bit(PORT6->P6MOD1, ((0x0FUL << 8U) | (0x0FUL << 0U)), PHOTO_COUPLER_INPUT_CONFIG);
    return true;
}

/** =================================================================*
 * @brief  液面センサーのLowアクティブ出力を読み取る。
 * @return 液面検出時はtrue
 * ================================================================= */
bool TankLevelSensor_IsLiquidDetected(void) {
    return (get_bit(PORT6->P6DI, SEN0204_INPUT_MASK) == 0U);
}
