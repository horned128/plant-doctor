/** =================================================================*
 * @file   PlantFeature.h
 * @brief  植物特徴量抽出API
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef PLANT_FEATURE_H
#define PLANT_FEATURE_H
#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */

#define PLANT_FEATURE_VALID_SOIL_PERMILLE      (1U << 0)
#define PLANT_FEATURE_VALID_SOIL_MA            (1U << 1)
#define PLANT_FEATURE_VALID_SOIL_RATE          (1U << 2)
#define PLANT_FEATURE_VALID_LEAF_AIR_DELTA     (1U << 3)
#define PLANT_FEATURE_VALID_LEAF_RATE          (1U << 4)
#define PLANT_FEATURE_VALID_ILLUMINANCE_RAW    (1U << 5)
#define PLANT_FEATURE_VALID_ILLUMINANCE_ACCUM  (1U << 6)
#define PLANT_FEATURE_VALID_HUMIDITY           (1U << 7)

#define PLANT_FEATURE_BUFFER_SIZE              (60U)
#define PLANT_FEATURE_MINUTE_HISTORY_SIZE      (60U)
#define PLANT_FEATURE_ACCUM_HOURS              (24U)

typedef struct {
    int32_t soilMoisturePermille;                           /* 校正後の相対湿り度 0〜1000 */
    int32_t soilMoistureMovingAverage;                      /* 直近N分の移動平均 */
    int32_t soilMoistureRatePerHour;                        /* 土壌水分の低下速度（1時間あたり） */
    int32_t leafAirTemperatureDelta;                        /* 葉温－気温 [摂氏1/100] */
    int32_t leafTemperatureRatePerHour;                     /* 葉温の上昇速度（1時間あたり） */
    int32_t illuminanceRaw;                                 /* SEN0228生値 */
    int32_t illuminanceAccumulated;                         /* 積算照度（日照不足判定用） */
    int32_t relativeHumidityCentiPercent;                   /* 相対湿度の1/100 */
    uint32_t elapsedSinceWateringSeconds;                   /* 前回給水からの経過秒 */
    uint8_t validMask;                                      /* 各特徴量の有効ビット */
} PLANT_FEATURE_VECTOR;

typedef struct {
    int32_t soilMoisturePermille;                           /* 校正済み土壌湿り度(0-1000‰) */
    int16_t leafTemperatureCentiC;                          /* 葉温 [1/100 ℃] */
    int16_t airTemperatureCentiC;                           /* 気温 [1/100 ℃] */
    uint16_t relativeHumidityCentiPercent;                  /* 相対湿度 [1/100 %RH] */
    uint16_t illuminanceRaw;                                /* 照度生値 */
    bool soilMoistureValid;                                 /* 土壌水分有効フラグ */
    bool leafTemperatureValid;                              /* 葉温有効フラグ */
    bool airTemperatureValid;                               /* 気温有効フラグ */
    bool illuminanceValid;                                  /* 照度有効フラグ */
    bool relativeHumidityValid;                             /* 湿度有効フラグ */
    bool wateringOccurred;                                  /* 給水イベント発生フラグ */
} PLANT_FEATURE_INPUT;

typedef struct {
    int16_t soilBuffer[PLANT_FEATURE_BUFFER_SIZE];
    bool soilValidBuffer[PLANT_FEATURE_BUFFER_SIZE];
    int16_t leafBuffer[PLANT_FEATURE_BUFFER_SIZE];
    bool leafValidBuffer[PLANT_FEATURE_BUFFER_SIZE];
    uint8_t secondIndex;
    uint8_t secondCount;

    int16_t soilMinuteHistory[PLANT_FEATURE_MINUTE_HISTORY_SIZE];
    bool soilMinuteValid[PLANT_FEATURE_MINUTE_HISTORY_SIZE];
    int16_t leafMinuteHistory[PLANT_FEATURE_MINUTE_HISTORY_SIZE];
    bool leafMinuteValid[PLANT_FEATURE_MINUTE_HISTORY_SIZE];
    uint8_t minuteHistoryCount;
    uint8_t minuteHistoryIndex;

    int32_t hourlyIlluminance[PLANT_FEATURE_ACCUM_HOURS];
    int32_t currentHourIlluminanceSum;
    uint8_t minuteInHour;
    uint8_t hourHistoryCount;
    uint8_t hourHistoryIndex;
    bool hasAccumulatedIlluminance;

    uint32_t elapsedSinceWateringSeconds;
    PLANT_FEATURE_VECTOR vector;
} PLANT_FEATURE_STATE;

void PlantFeature_Reset(PLANT_FEATURE_STATE *state);        /* 特徴量状態初期化 */
void PlantFeature_Update(PLANT_FEATURE_STATE *state, const PLANT_FEATURE_INPUT *input); /* 1サンプル分更新 */
void PlantFeature_GetVector(const PLANT_FEATURE_STATE *state, PLANT_FEATURE_VECTOR *vector); /* ベクトル取得 */

#endif /* PLANT_FEATURE_H */
