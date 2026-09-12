/** =================================================================*
 * @file   WateringResponse.h
 * @brief  水やり後自己診断・土壌劣化推定API (P6-1, P6-2)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef WATERING_RESPONSE_H
#define WATERING_RESPONSE_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"              /* アプリケーション状態型 */
#include "PumpControl.h"                    /* PUMP_WATERING_EVENT型 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t immediateSeconds;             /* 給水直後測定時間 [秒] (既定30) */
    uint32_t delayedSeconds;               /* 浸透後測定時間 [秒] (既定180) */
    uint32_t tempDeltaSeconds;             /* 蒸散回復測定時間 [秒] (既定300) */
    uint32_t timeoutSeconds;               /* 測定上限タイムアウト [秒] (既定600) */
    uint32_t immediateTicks;               /* 直後測定時間 [ticks] */
    uint32_t delayedTicks;                 /* 浸透後測定時間 [ticks] */
    uint32_t tempDeltaTicks;               /* 蒸散回復測定時間 [ticks] */
    uint32_t timeoutTicks;                 /* タイムアウト [ticks] */
    int16_t minSoilRecoveryPermille;       /* 水分回復判定最小増加量 [‰] */
    int16_t minTempDropCentic;             /* 葉温回復判定最小低下量 [1/100 ℃] */
    uint8_t degradeRequiredCycles;         /* 劣化判定に必要な連続サイクル数 (既定3) */
    int16_t degradeMinGainPermille;        /* 保水性低下判定の増加量下限 [‰] */
} WATERING_RESPONSE_CONFIG;

typedef struct {
    int16_t soilMoisturePermille;          /* 現在の土壌水分千分率 [‰] */
    int16_t leafAirDeltaCentic;            /* 現在の葉温気温差 [1/100 ℃] */
    bool soilMoistureValid;                /* 土壌水分有効フラグ */
    bool leafAirDeltaValid;                /* 葉温気温差有効フラグ */
    uint32_t nowSeconds;                   /* 現在UNIX秒 (未同期時は0xFFFFFFFF) */
    uint32_t currentTick;                  /* 現在の10ms Tick */
} WATERING_RESPONSE_INPUT;

typedef struct {
    WATERING_RESPONSE status;              /* 現在の応答評価結果 */
    PUMP_WATERING_EVENT wateringEvent;     /* 評価対象の給水イベント */
    uint32_t startTick;                    /* 給水完了時点のTick */
    int16_t delayedGainPermille;           /* 浸透後の土壌水分増加量 */
    int16_t tempDropCentic;                /* 葉温気温差の低下量 */
    bool didNotCoolLeaf;                   /* 給水後も葉温が下がらないフラグ */
    bool isSoilDegraded;                   /* 土壌劣化フラグ (P6-2) */
    uint8_t consecutiveDegradeCycles;      /* 連続土壌劣化サイクル数 */
    bool delayedMeasured;                  /* delayed時点測定完了フラグ */
    bool tempMeasured;                     /* tempDelta時点測定完了フラグ */
} WATERING_RESPONSE_STATE;

void WateringResponse_Reset(WATERING_RESPONSE_STATE *state);
void WateringResponse_NotifyWatering(WATERING_RESPONSE_STATE *state,
                                     const PUMP_WATERING_EVENT *event,
                                     uint32_t currentTick);
void WateringResponse_Update(WATERING_RESPONSE_STATE *state,
                             const WATERING_RESPONSE_INPUT *input,
                             const WATERING_RESPONSE_CONFIG *config);
WATERING_RESPONSE WateringResponse_GetResult(const WATERING_RESPONSE_STATE *state);
bool WateringResponse_DidNotCoolLeaf(const WATERING_RESPONSE_STATE *state);
bool WateringResponse_IsSoilDegraded(const WATERING_RESPONSE_STATE *state);
void WateringResponse_ClearFailure(WATERING_RESPONSE_STATE *state);

#ifdef __cplusplus
}
#endif

#endif /* WATERING_RESPONSE_H */
