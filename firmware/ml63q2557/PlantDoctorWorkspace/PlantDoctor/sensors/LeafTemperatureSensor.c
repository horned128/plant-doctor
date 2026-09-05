/** =================================================================*
 * @file   LeafTemperatureSensor.c
 * @brief  葉温センサー
 * ================================================================= */
#include "LeafTemperatureSensor.h"                          /* LeafTemperatureSensorのAPIと型定義 */

/** =================================================================*
 * @brief  LeafTemperatureSensor_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool LeafTemperatureSensor_Init(void) {
    /* 赤外線センサーとバス割り当ては未選定である。 */
    return true;
}

/** =================================================================*
 * @brief  LeafTemperatureSensor_Read処理
 * @param[out] temperatureCentiC 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool LeafTemperatureSensor_Read(int16_t *temperatureCentiC) {
    (void)temperatureCentiC;
    return false;
}
