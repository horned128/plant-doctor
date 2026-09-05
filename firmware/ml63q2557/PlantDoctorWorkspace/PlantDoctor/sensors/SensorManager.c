/** =================================================================*
 * @file   SensorManager.c
 * @brief  センサー管理
 * ================================================================= */
#include "SensorManager.h"                                  /* SensorManagerのAPIと型定義 */
#include "EnvironmentSensor.h"                              /* EnvironmentSensorのAPIと型定義 */
#include "LeafTemperatureSensor.h"                          /* LeafTemperatureSensorのAPIと型定義 */
#include "SoilMoistureSensor.h"                             /* SoilMoistureSensorのAPIと型定義 */

static PLANT_SENSOR_SNAPSHOT s_latest;                      /**< モジュール内部状態 */

/** =================================================================*
 * @brief  SensorManager_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool SensorManager_Init(void) {
    s_latest.soilMoistureRaw = 0U;
    s_latest.leafTemperatureCentiC = 0;
    s_latest.airTemperatureCentiC = 0;
    s_latest.relativeHumidityCentiPercent = 0U;
    s_latest.illuminanceRaw = 0U;
    s_latest.valid = false;

    return SoilMoistureSensor_Init() &&
        LeafTemperatureSensor_Init() &&
        EnvironmentSensor_Init();
}

/** =================================================================*
 * @brief  SensorManager_Process10Ms処理
 * ================================================================= */
void SensorManager_Process10Ms(void) {
    /* 将来の非ブロッキングセンサードライバとのスケジュール境界とする。 */
}

/** =================================================================*
 * @brief  SensorManager_GetLatest処理
 * @param[out] snapshot 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool SensorManager_GetLatest(PLANT_SENSOR_SNAPSHOT *snapshot) {
    if ((snapshot == 0) || !s_latest.valid) {
        return false;
    }
    *snapshot = s_latest;
    return true;
}
