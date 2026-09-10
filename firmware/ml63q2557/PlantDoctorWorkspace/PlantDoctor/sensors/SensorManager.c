/** =================================================================*
 * @file   SensorManager.c
 * @brief  センサー管理
 * ================================================================= */
#include "SensorManager.h"                                  /* SensorManagerのAPIと型定義 */
#include "EnvironmentSensor.h"                              /* EnvironmentSensorのAPIと型定義 */
#include "LeafTemperatureSensor.h"                          /* LeafTemperatureSensorのAPIと型定義 */
#include "SoilMoistureSensor.h"                             /* SoilMoistureSensorのAPIと型定義 */
#include "TankLevelSensor.h"                                /* TankLevelSensorのAPIと型定義 */
#include "PlantDoctorConfig.h"                              /* PlantDoctorConfigのAPIと型定義 */

static PLANT_SENSOR_SNAPSHOT s_latest;                      /**< モジュール内部状態 */
static uint16_t s_sampleTicks;                              /**< 次回サンプルまでのTick */
static bool s_samplePending;                                /**< サンプル実行要求 */

/** =================================================================*
 * @brief  SensorManager_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool SensorManager_Init(void) {
    s_latest.soilMoistureRaw = 0U;
    s_latest.leafTemperatureCentiC = 0;
    s_latest.airTemperatureCentiC = 0;
    s_latest.relativeHumidityCentiPercent = 0U;
    s_latest.barometricPressurePa = 0UL;
    s_latest.illuminanceCentiLux = 0UL;
    s_latest.illuminanceRaw = 0U;
    s_latest.soilMoistureValid = false;
    s_latest.leafTemperatureValid = false;
    s_latest.airTemperatureValid = false;
    s_latest.barometricPressureValid = false;
    s_latest.illuminanceValid = false;
    s_latest.tankLiquidDetected = false;
    s_latest.valid = false;
    s_sampleTicks = PLANT_DOCTOR_SENSOR_SAMPLE_TICKS;
    s_samplePending = false;

    return SoilMoistureSensor_Init() && LeafTemperatureSensor_Init() &&
        EnvironmentSensor_Init() && TankLevelSensor_Init();
}

/** =================================================================*
 * @brief  SensorManager_Process10Ms処理
 * ================================================================= */
void SensorManager_Process10Ms(void) {
    ENVIRONMENT_SENSOR_SAMPLE environment;

    if (s_sampleTicks > 0U) {
        --s_sampleTicks;
    }
    if (s_sampleTicks == 0U) {
        s_samplePending = true;
    }
    if (!s_samplePending) {
        return;
    }

    s_samplePending = false;
    s_sampleTicks = PLANT_DOCTOR_SENSOR_SAMPLE_TICKS;
    s_latest.soilMoistureValid = SoilMoistureSensor_Read(&s_latest.soilMoistureRaw);
    s_latest.leafTemperatureValid = LeafTemperatureSensor_Read(&s_latest.leafTemperatureCentiC);
    (void)EnvironmentSensor_Read(&environment);
    s_latest.airTemperatureCentiC = environment.airTemperatureCentiC;
    s_latest.relativeHumidityCentiPercent = environment.relativeHumidityCentiPercent;
    s_latest.barometricPressurePa = environment.barometricPressurePa;
    s_latest.illuminanceCentiLux = environment.illuminanceCentiLux;
    s_latest.illuminanceRaw = environment.illuminanceRaw;
    s_latest.airTemperatureValid = environment.airTemperatureValid;
    s_latest.barometricPressureValid = environment.barometricPressureValid;
    s_latest.illuminanceValid = environment.illuminanceValid;
    s_latest.tankLiquidDetected = TankLevelSensor_IsLiquidDetected();
    /* 未接続センサーは無効値として表示し、起動・他センサーの計測を継続する。 */
    s_latest.valid = true;
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
