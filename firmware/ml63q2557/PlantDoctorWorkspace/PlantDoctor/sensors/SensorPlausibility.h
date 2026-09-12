/** =================================================================*
 * @file   SensorPlausibility.h
 * @brief  センサー妥当性・整合性判定API
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef SENSOR_PLAUSIBILITY_H
#define SENSOR_PLAUSIBILITY_H
#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"              /* SENSOR_HEALTH, SENSOR_HEALTH_REPORT などの状態型 */

typedef struct {
    uint16_t soilMoistureRaw;                               /* 土壌水分生値 */
    bool soilMoistureValid;                                 /* 土壌水分通信有効 */
    int16_t leafTemperatureCentiC;                          /* 葉温 */
    bool leafTemperatureValid;                              /* 葉温通信有効 */
    int16_t airTemperatureCentiC;                           /* 気温 */
    bool airTemperatureValid;                               /* 気温通信有効 */
    uint16_t relativeHumidityCentiPercent;                  /* 湿度 */
    bool relativeHumidityValid;                             /* 湿度通信有効 */
    uint16_t illuminanceRaw;                                /* 照度生値 */
    bool illuminanceValid;                                  /* 照度通信有効 */
    uint16_t soilDryCalibration;                            /* 校正Dry生値 */
    uint16_t soilWetCalibration;                            /* 校正Wet生値 */
} SENSOR_PLAUSIBILITY_INPUT;

typedef struct {
    uint8_t tempDeltaInconsistentCount;
    uint8_t luxZeroCount;
    uint16_t lastSoilRaw;
    bool hasLastSoilRaw;
    SENSOR_HEALTH_REPORT report;
} SENSOR_PLAUSIBILITY_STATE;

void SensorPlausibility_Reset(SENSOR_PLAUSIBILITY_STATE *state); /* 判定状態初期化 */
void SensorPlausibility_Evaluate(SENSOR_PLAUSIBILITY_STATE *state, const SENSOR_PLAUSIBILITY_INPUT *input); /* 妥当性判定更新 */
void SensorPlausibility_GetReport(const SENSOR_PLAUSIBILITY_STATE *state, SENSOR_HEALTH_REPORT *report); /* 判定結果取得 */

#endif /* SENSOR_PLAUSIBILITY_H */
