/** =================================================================*
 * @file   EnvironmentSensor.c
 * @brief  環境センサー
 * ================================================================= */
#include "EnvironmentSensor.h"                              /* EnvironmentSensorのAPIと型定義 */

/** =================================================================*
 * @brief  EnvironmentSensor_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool EnvironmentSensor_Init(void) {
    /* 温度、湿度、照度センサーの型番は未選定である。 */
    return true;
}

/** =================================================================*
 * @brief  EnvironmentSensor_Read処理
 * @param[out] sample 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool EnvironmentSensor_Read(ENVIRONMENT_SENSOR_SAMPLE *sample) {
    (void)sample;
    return false;
}
