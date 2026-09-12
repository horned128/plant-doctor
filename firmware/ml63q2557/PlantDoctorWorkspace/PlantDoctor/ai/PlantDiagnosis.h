/** =================================================================*
 * @file   PlantDiagnosis.h
 * @brief  植物診断・原因候補推定API (P4-2)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef PLANT_DIAGNOSIS_H
#define PLANT_DIAGNOSIS_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"              /* 状態・トレンド型 */
#include "PlantFeature.h"                   /* 特徴量ベクトル型 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DIAGNOSIS_FAILED_SENSOR_NONE = 0,
    DIAGNOSIS_FAILED_SENSOR_SOIL,
    DIAGNOSIS_FAILED_SENSOR_LEAF,
    DIAGNOSIS_FAILED_SENSOR_AIR_HUM,
    DIAGNOSIS_FAILED_SENSOR_LUX
} DIAGNOSIS_FAILED_SENSOR;

typedef struct {
    int32_t dryThresholdPermille;          /* 土壌乾燥しきい値 [‰] */
    int32_t heatStressTempDelta;           /* 熱ストレス葉温気温差 [1/100 ℃] */
    int32_t heatStressRatePerHour;         /* 熱ストレス葉温上昇速度 [1/100 ℃/h] */
    int32_t lowLightAccumulatedThreshold;  /* 日照不足積算照度しきい値 */
    int32_t rootUptakeSoilMinPermille;     /* 吸水不良判定の土壌水分下限 [‰] */
    int32_t rootUptakeTempDelta;           /* 吸水不良葉温気温差 [1/100 ℃] */
    int32_t soilDryRatePerHour;            /* 土壌乾燥傾向速度 [‰/h] */
    int32_t soilWetRatePerHour;            /* 土壌湿潤傾向速度 [‰/h] */
} PLANT_DIAGNOSIS_CONFIG;

typedef struct {
    PLANT_FEATURE_VECTOR features;         /* P1-3の特徴量ベクトル */
    SENSOR_HEALTH soilHealth;              /* 土壌センサ健全性 */
    SENSOR_HEALTH leafHealth;              /* 葉温センサ健全性 */
    SENSOR_HEALTH airHumHealth;            /* 温湿度センサ健全性 */
    SENSOR_HEALTH luxHealth;               /* 照度センサ健全性 */
    bool isWateringActive;                 /* 給水動作中フラグ */
    bool isWateringFailed;                 /* 給水失敗フラグ (P6-1) */
    bool isSoilDegraded;                   /* 土壌劣化フラグ (P6-2) */
    bool wateringDidNotCoolLeaf;           /* 給水後も葉温が下がらないフラグ (P6-1) */
} PLANT_DIAGNOSIS_INPUT;

typedef struct {
    PLANT_STATUS status;
    SOIL_TREND soilTrend;
    DIAGNOSIS_FAILED_SENSOR failedSensor;
} PLANT_DIAGNOSIS_STATE;

void PlantDiagnosis_Reset(PLANT_DIAGNOSIS_STATE *state);
void PlantDiagnosis_Update(PLANT_DIAGNOSIS_STATE *state,
                           const PLANT_DIAGNOSIS_INPUT *input,
                           const PLANT_DIAGNOSIS_CONFIG *config);
PLANT_STATUS PlantDiagnosis_GetStatus(const PLANT_DIAGNOSIS_STATE *state);
SOIL_TREND PlantDiagnosis_GetSoilTrend(const PLANT_DIAGNOSIS_STATE *state);
DIAGNOSIS_FAILED_SENSOR PlantDiagnosis_GetFailedSensor(const PLANT_DIAGNOSIS_STATE *state);

#ifdef __cplusplus
}
#endif

#endif /* PLANT_DIAGNOSIS_H */
