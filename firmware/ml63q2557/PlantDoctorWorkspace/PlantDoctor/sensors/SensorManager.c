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
#include "TimeKeeper.h"                                     /* TimeKeeperのAPIと型定義 */

static PLANT_SENSOR_SNAPSHOT s_latest;                      /**< モジュール内部状態 */
static uint16_t s_sampleTicks;                              /**< 次回サンプルまでのTick */
static bool s_samplePending;                                /**< サンプル実行要求 */
static uint16_t s_sampleSequence;                           /**< サンプル連番カウンタ */
static uint16_t s_lastTakenSequence;                        /**< 前回取得した連番 */
static bool s_hasTakenSample;                               /**< サンプル取得済みフラグ */

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
    s_latest.timestampSeconds = 0xFFFFFFFFUL;
    s_latest.sampleSequence = 0U;
    s_sampleTicks = PLANT_DOCTOR_SENSOR_SAMPLE_TICKS;
    s_samplePending = false;
    s_sampleSequence = 0U;
    s_lastTakenSequence = 0U;
    s_hasTakenSample = false;

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
    ++s_sampleSequence;
    s_latest.sampleSequence = s_sampleSequence;
    s_latest.timestampSeconds = TimeKeeper_GetUnixSeconds();
    /* 未接続センサーは無効値として表示し、起動・他センサーの計測を継続する。 */
    s_latest.valid = true;
}

/** =================================================================*
 * @brief  SensorManager_GetLatest処理
 * @param[out] snapshot 最新スナップショット出力先
 * @return 実行結果または取得値
 * ================================================================= */
bool SensorManager_GetLatest(PLANT_SENSOR_SNAPSHOT *snapshot) {
    if ((snapshot == 0) || !s_latest.valid) {
        return false;
    }
    *snapshot = s_latest;
    return true;
}

/** =================================================================*
 * @brief  新規サンプルのみの取得
 * @param[out] snapshot 最新スナップショット出力先
 * @return 新規サンプルが取得できた場合true、未更新ならfalse
 * ================================================================= */
bool SensorManager_TakeNewSample(PLANT_SENSOR_SNAPSHOT *snapshot) {
    if ((snapshot == 0) || !s_latest.valid) {
        return false;
    }
    if (!s_hasTakenSample || (s_latest.sampleSequence != s_lastTakenSequence)) {
        s_hasTakenSample = true;
        s_lastTakenSequence = s_latest.sampleSequence;
        *snapshot = s_latest;
        return true;
    }
    return false;
}

/** =================================================================*
 * @brief  最新サンプルのsampleSequence取得
 * @return 最新サンプルの連番
 * ================================================================= */
uint16_t SensorManager_GetSampleSequence(void) {
    return s_latest.sampleSequence;
}

