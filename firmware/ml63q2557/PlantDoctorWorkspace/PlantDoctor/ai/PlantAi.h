/** =================================================================*
 * @file   PlantAi.h
 * @brief  植物状態AI API
 * ================================================================= */
#ifndef PLANT_AI_H
#define PLANT_AI_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"                              /* PLANT_STATUS, SOIL_TREND */
#include "PlantDiagnosis.h"                                 /* DIAGNOSIS_FAILED_SENSOR */
#include "PlantFeature.h"                                   /* 植物特徴量型とAPI */
#include "PumpControl.h"                                    /* 給水イベント型定義 */

bool PlantAi_Init(void);                                    /* PlantAi_InitのAPI */
void PlantAi_Process10Ms(void);                             /* PlantAi_Process10MsのAPI */
bool PlantAi_IsAnomaly(void);                               /* PlantAi_IsAnomalyのAPI */
void PlantAi_GetFeatureVector(PLANT_FEATURE_VECTOR *vector); /* 最新特徴量ベクトルの取得 */
PLANT_STATUS PlantAi_GetStatus(void);                       /* 最新診断ステータスの取得 */
SOIL_TREND PlantAi_GetSoilTrend(void);                      /* 最新土壌傾向の取得 */
DIAGNOSIS_FAILED_SENSOR PlantAi_GetFailedSensor(void);      /* 異常センサーグループの取得 */
uint8_t PlantAi_GetStressScore(void);                       /* 最新ストレススコア(0-100)の取得 */
void PlantAi_NotifyWatering(const PUMP_WATERING_EVENT *event, uint32_t currentTick); /* 給水イベント通知 */
WATERING_RESPONSE PlantAi_GetWateringResponse(void);        /* 最新給水応答結果の取得 */
void PlantAi_ClearWateringFailure(void);                    /* 給水失敗状態の手動解除 */

#endif /* PLANT_AI_H */

