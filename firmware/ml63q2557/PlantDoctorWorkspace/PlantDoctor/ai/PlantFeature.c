/** =================================================================*
 * @file   PlantFeature.c
 * @brief  植物特徴量
 * ================================================================= */
#include "PlantFeature.h"                                   /* PlantFeatureのAPIと型定義 */

/** =================================================================*
 * @brief  PlantFeature_Reset処理
 * @param[out] feature 引数
 * ================================================================= */
void PlantFeature_Reset(PLANT_FEATURE_VECTOR *feature) {
    if (feature != 0) {
        feature->soilMoisture = 0;
        feature->soilMoistureMovingAverage = 0;
        feature->soilMoistureRate = 0;
        feature->leafAirTemperatureDelta = 0;
        feature->elapsedSinceWateringSeconds = 0U;
    }
}
