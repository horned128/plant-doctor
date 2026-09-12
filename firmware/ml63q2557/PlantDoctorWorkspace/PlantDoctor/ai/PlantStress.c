/** =================================================================*
 * @file   PlantStress.c
 * @brief  植物ストレスバロメータ 0〜100算出実装 (P4-3)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "PlantStress.h"
#include <stddef.h>

/** =================================================================*
 * @brief  植物ストレスバロメータ（0〜100）を算出
 * @param  features 特徴量ベクトル
 * @param  health   センサー健全性レポート
 * @param  config   ストレス計算設定（平常値・重み）
 * @param  output   算出結果出力
 * ================================================================= */
void PlantStress_Evaluate(const PLANT_FEATURE_VECTOR *features,
                          const SENSOR_HEALTH_REPORT *health,
                          const PLANT_STRESS_CONFIG *config,
                          PLANT_STRESS_OUTPUT *output) {
    int32_t deviation;
    int32_t base;
    int32_t finalScore;
    int32_t maxWeightedPartial = 0;
    int32_t otherWeightedSum = 0;
    int32_t count = 0;
    int32_t secondaryFactor;
    int32_t secondaryAdd = 0;

    if ((features == NULL) || (health == NULL) || (config == NULL) || (output == NULL)) {
        return;
    }

    output->stressScore = PLANT_STRESS_UNKNOWN;
    output->evaluatedFeaturesMask = 0U;
    output->soilPartialScore = 0U;
    output->heatPartialScore = 0U;
    output->lightPartialScore = 0U;
    output->humidityPartialScore = 0U;

    /* 1. 健全性および有効ビットに基づき、評価可能な特徴量グループを選定 */
    if ((health->soilHealth == SENSOR_HEALTH_OK) &&
        ((features->validMask & PLANT_FEATURE_VALID_SOIL_PERMILLE) != 0U)) {
        output->evaluatedFeaturesMask |= PLANT_STRESS_FEATURE_SOIL;
    }

    if ((health->leafHealth == SENSOR_HEALTH_OK) &&
        (health->airHumHealth == SENSOR_HEALTH_OK) &&
        ((features->validMask & PLANT_FEATURE_VALID_LEAF_AIR_DELTA) != 0U)) {
        output->evaluatedFeaturesMask |= PLANT_STRESS_FEATURE_HEAT;
    }

    if ((health->luxHealth == SENSOR_HEALTH_OK) &&
        ((features->validMask & PLANT_FEATURE_VALID_ILLUMINANCE_ACCUM) != 0U)) {
        output->evaluatedFeaturesMask |= PLANT_STRESS_FEATURE_LIGHT;
    }

    if ((health->airHumHealth == SENSOR_HEALTH_OK) &&
        ((features->validMask & PLANT_FEATURE_VALID_HUMIDITY) != 0U)) {
        output->evaluatedFeaturesMask |= PLANT_STRESS_FEATURE_HUMIDITY;
    }

    /* 有効な特徴量が1つもない場合は判定不能 */
    if (output->evaluatedFeaturesMask == 0U) {
        output->stressScore = PLANT_STRESS_UNKNOWN;
        return;
    }

    /* 2. 各評価対象特徴量の部分スコア（0〜100）を算出 */
    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_SOIL) != 0U) {
        if ((config->baselineSoilMoisturePermille > 0) &&
            (features->soilMoisturePermille < config->baselineSoilMoisturePermille)) {
            int32_t diff = config->baselineSoilMoisturePermille - features->soilMoisturePermille;
            int32_t score = (diff * 100L) / config->baselineSoilMoisturePermille;
            if (score > 100L) {
                score = 100L;
            }
            output->soilPartialScore = (uint8_t)score;
        } else {
            output->soilPartialScore = 0U;
        }
    }

    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_HEAT) != 0U) {
        if ((config->maxHeatDeltaRangeCentiC > 0) &&
            (features->leafAirTemperatureDelta > config->baselineTempDeltaCentiC)) {
            int32_t diff = features->leafAirTemperatureDelta - config->baselineTempDeltaCentiC;
            int32_t score = (diff * 100L) / config->maxHeatDeltaRangeCentiC;
            if (score > 100L) {
                score = 100L;
            }
            output->heatPartialScore = (uint8_t)score;
        } else {
            output->heatPartialScore = 0U;
        }
    }

    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_LIGHT) != 0U) {
        if ((config->baselineIlluminanceAccum > 0) &&
            (features->illuminanceAccumulated < config->baselineIlluminanceAccum)) {
            int32_t diff = config->baselineIlluminanceAccum - features->illuminanceAccumulated;
            int32_t score = (diff * 100L) / config->baselineIlluminanceAccum;
            if (score > 100L) {
                score = 100L;
            }
            output->lightPartialScore = (uint8_t)score;
        } else {
            output->lightPartialScore = 0U;
        }
    }

    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_HUMIDITY) != 0U) {
        if ((config->baselineHumidityCentiPercent > 0U) &&
            (features->relativeHumidityCentiPercent < config->baselineHumidityCentiPercent)) {
            int32_t diff = (int32_t)config->baselineHumidityCentiPercent - (int32_t)features->relativeHumidityCentiPercent;
            int32_t score = (diff * 100L) / (int32_t)config->baselineHumidityCentiPercent;
            if (score > 100L) {
                score = 100L;
            }
            output->humidityPartialScore = (uint8_t)score;
        } else {
            output->humidityPartialScore = 0U;
        }
    }

    /* 3. 重み付き最大値による合成（最大値支配＋副次特徴量の加算） */
    maxWeightedPartial = 0;
    otherWeightedSum = 0;
    count = 0;

    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_SOIL) != 0U) {
        int32_t weighted = ((int32_t)output->soilPartialScore * (int32_t)config->weightSoil) / 100;
        if (weighted > maxWeightedPartial) {
            otherWeightedSum += maxWeightedPartial;
            maxWeightedPartial = weighted;
        } else {
            otherWeightedSum += weighted;
        }
        count++;
    }

    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_HEAT) != 0U) {
        int32_t weighted = ((int32_t)output->heatPartialScore * (int32_t)config->weightHeat) / 100;
        if (weighted > maxWeightedPartial) {
            otherWeightedSum += maxWeightedPartial;
            maxWeightedPartial = weighted;
        } else {
            otherWeightedSum += weighted;
        }
        count++;
    }

    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_LIGHT) != 0U) {
        int32_t weighted = ((int32_t)output->lightPartialScore * (int32_t)config->weightLight) / 100;
        if (weighted > maxWeightedPartial) {
            otherWeightedSum += maxWeightedPartial;
            maxWeightedPartial = weighted;
        } else {
            otherWeightedSum += weighted;
        }
        count++;
    }

    if ((output->evaluatedFeaturesMask & PLANT_STRESS_FEATURE_HUMIDITY) != 0U) {
        int32_t weighted = ((int32_t)output->humidityPartialScore * (int32_t)config->weightHumidity) / 100;
        if (weighted > maxWeightedPartial) {
            otherWeightedSum += maxWeightedPartial;
            maxWeightedPartial = weighted;
        } else {
            otherWeightedSum += weighted;
        }
        count++;
    }

    /* 単一深刻ストレスが薄まらないよう最大値を主軸とし、他のストレスがあれば加算 */
    secondaryFactor = 100 - (int32_t)config->maxDominanceWeight;
    secondaryAdd = 0;
    if (count > 1) {
        secondaryAdd = (otherWeightedSum * secondaryFactor) / (100 * (count - 1));
    }
    deviation = maxWeightedPartial + secondaryAdd;
    if (deviation > 100) {
        deviation = 100;
    }

    /* 4. ベーススコアからのスケーリング */
    base = (int32_t)config->baseStressScore;
    if (base > 100) {
        base = 100;
    }
    finalScore = base + (((100 - base) * deviation) / 100);
    if (finalScore > 100) {
        finalScore = 100;
    }

    output->stressScore = (uint8_t)finalScore;
}

