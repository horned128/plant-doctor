/** =================================================================*
 * @file   PlantStress.h
 * @brief  植物ストレスバロメータ 0〜100算出API (P4-3)
 * @author Plant Doctor team
 * @date   2026-09
 *
 * @note   0〜100の定義は「平常状態からのずれ」。
 *         病気の確率や健康度ではありません。
 * ================================================================= */
#ifndef PLANT_STRESS_H
#define PLANT_STRESS_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"              /* 状態型 (PLANT_STRESS_UNKNOWN, SENSOR_HEALTH) */
#include "PlantFeature.h"                   /* 特徴量ベクトル型 */

#ifdef __cplusplus
extern "C" {
#endif

#define PLANT_STRESS_FEATURE_SOIL       (1U << 0)
#define PLANT_STRESS_FEATURE_HEAT       (1U << 1)
#define PLANT_STRESS_FEATURE_LIGHT      (1U << 2)
#define PLANT_STRESS_FEATURE_HUMIDITY   (1U << 3)

typedef struct {
    uint8_t baseStressScore;               /* 平常時ベーススコア (例: 18) */
    uint8_t weightSoil;                    /* 土壌水分の重み */
    uint8_t weightHeat;                    /* 熱ストレスの重み */
    uint8_t weightLight;                   /* 日照の重み */
    uint8_t weightHumidity;                /* 湿度の重み */
    uint8_t maxDominanceWeight;            /* 最大値支配度[%] (75 -> 75% max, 25% avg) */

    int32_t baselineSoilMoisturePermille;  /* 平常土壌水分 [‰] */
    int32_t baselineTempDeltaCentiC;       /* 平常葉温気温差 [1/100 ℃] */
    int32_t maxHeatDeltaRangeCentiC;       /* 熱ストレスフルスケール差分 [1/100 ℃] */
    int32_t baselineIlluminanceAccum;      /* 平常積算照度 */
    uint16_t baselineHumidityCentiPercent; /* 平常湿度 [%RH 1/100] */
} PLANT_STRESS_CONFIG;

typedef struct {
    uint8_t stressScore;                   /* 0〜100、または PLANT_STRESS_UNKNOWN (0xFF) */
    uint8_t evaluatedFeaturesMask;         /* 合成に寄与した特徴量マスク */
    uint8_t soilPartialScore;              /* 土壌水分部分スコア (0-100) */
    uint8_t heatPartialScore;              /* 熱ストレス部分スコア (0-100) */
    uint8_t lightPartialScore;             /* 日照部分スコア (0-100) */
    uint8_t humidityPartialScore;          /* 湿度部分スコア (0-100) */
} PLANT_STRESS_OUTPUT;

void PlantStress_Evaluate(const PLANT_FEATURE_VECTOR *features,
                          const SENSOR_HEALTH_REPORT *health,
                          const PLANT_STRESS_CONFIG *config,
                          PLANT_STRESS_OUTPUT *output);

#ifdef __cplusplus
}
#endif

#endif /* PLANT_STRESS_H */
