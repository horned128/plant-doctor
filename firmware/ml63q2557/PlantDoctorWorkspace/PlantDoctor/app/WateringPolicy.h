/** =================================================================*
 * @file   WateringPolicy.h
 * @brief  自律水やり判定ポリシーAPI (P5-5)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef WATERING_POLICY_H
#define WATERING_POLICY_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"              /* アプリケーション状態型 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WATERING_DECISION_HOLD = 0,             /* 給水不要（非乾燥） */
    WATERING_DECISION_REQUEST,              /* 給水要求（全条件充足） */
    WATERING_DECISION_BLOCKED               /* 給水保留（乾燥だが安全インターロック・制約により保留） */
} WATERING_DECISION;

typedef struct {
    int16_t dryThresholdPermille;          /* 乾燥判定しきい値 [‰] */
    uint32_t minIntervalSeconds;           /* 最小給水間隔 [秒] */
    uint32_t minIntervalTicks;             /* 最小給水間隔 [10ms ticks] */
    uint8_t maxDailyWateringCount;         /* 1日あたりの最大給水回数 */
    bool autoWateringEnabled;              /* 自律水やり有効フラグ */
} WATERING_POLICY_CONFIG;

typedef struct {
    PLANT_STATUS plantStatus;              /* 現在の植物診断状態 */
    int16_t soilMoisturePermille;          /* 校正後の土壌水分 [‰] */
    SENSOR_HEALTH soilSensorHealth;        /* 土壌センサ健全性 */
    bool tankLiquidDetected;               /* タンク液面検出状態 */
    bool isMonitoring;                     /* MONITOR状態フラグ */
    WATERING_RESPONSE lastWateringResponse;/* 直近の給水応答自己診断結果 */
    uint32_t nowSeconds;                   /* 現在UNIX秒 (未同期時は0xFFFFFFFF) */
    uint32_t currentTick;                  /* 現在の10ms Tickカウンタ */
} WATERING_POLICY_INPUT;

typedef struct {
    uint32_t lastWateringSeconds;          /* 前回給水完了時のUNIX秒 */
    uint32_t lastWateringTick;             /* 前回給水完了時のTick */
    uint8_t dailyWateringCount;            /* 当日の累計給水回数 */
    uint32_t lastDayIndex;                 /* 日付インデックス (nowSeconds / 86400) */
} WATERING_POLICY_STATE;

void WateringPolicy_Reset(WATERING_POLICY_STATE *state);
WATERING_DECISION WateringPolicy_Evaluate(WATERING_POLICY_STATE *state,
                                         const WATERING_POLICY_INPUT *input,
                                         const WATERING_POLICY_CONFIG *config);
void WateringPolicy_NotifyWateringExecuted(WATERING_POLICY_STATE *state,
                                          uint32_t nowSeconds,
                                          uint32_t currentTick);

#ifdef __cplusplus
}
#endif

#endif /* WATERING_POLICY_H */
