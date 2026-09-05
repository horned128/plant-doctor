/** =================================================================*
 * @file   PlantFeature.h
 * @brief  植物特徴量型とAPI
 * ================================================================= */
#ifndef PLANT_FEATURE_H
#define PLANT_FEATURE_H

#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

typedef struct {
    int32_t soilMoisture;
    int32_t soilMoistureMovingAverage;
    int32_t soilMoistureRate;
    int32_t leafAirTemperatureDelta;
    uint32_t elapsedSinceWateringSeconds;
} PLANT_FEATURE_VECTOR;

void PlantFeature_Reset(PLANT_FEATURE_VECTOR *feature);     /* PlantFeature_ResetのAPI */

#endif /* PLANT_FEATURE_H */
