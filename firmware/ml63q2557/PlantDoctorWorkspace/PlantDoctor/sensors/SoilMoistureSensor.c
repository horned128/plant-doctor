/** =================================================================*
 * @file   SoilMoistureSensor.c
 * @brief  土壌水分センサー
 * ================================================================= */
#include "SoilMoistureSensor.h"                             /* SoilMoistureSensorのAPIと型定義 */

/** =================================================================*
 * @brief  SoilMoistureSensor_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool SoilMoistureSensor_Init(void) {
    /* センサー型番とADCチャンネルは意図的に未選定である。 */
    return true;
}

/** =================================================================*
 * @brief  SoilMoistureSensor_Read処理
 * @param[out] rawValue 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool SoilMoistureSensor_Read(uint16_t *rawValue) {
    (void)rawValue;
    return false;
}
