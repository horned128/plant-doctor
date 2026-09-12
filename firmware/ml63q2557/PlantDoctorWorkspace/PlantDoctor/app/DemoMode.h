/** =================================================================*
 * @file   DemoMode.h
 * @brief  デモモード基盤API (P8-1)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef DEMO_MODE_H
#define DEMO_MODE_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"              /* SENSOR_HEALTH等の型定義 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEMO_SCENARIO_OFF = 0,                  /* デモモード無効（通常動作） */
    DEMO_SCENARIO_1_NORMAL,                 /* DEMO1: 通常診断 (Healthy, 低ストレス) */
    DEMO_SCENARIO_2_HEAT_STRESS,            /* DEMO2: 日照・熱ストレス (+2.00℃, 照度高) */
    DEMO_SCENARIO_3_DRY_STRESS,             /* DEMO3: 乾燥・自律水やり (土壌水分低) */
    DEMO_SCENARIO_4_WATERING_FAILED,        /* DEMO4: 水やり失敗 (給水後も水分不回復) */
    DEMO_SCENARIO_5_SENSOR_ERROR            /* DEMO5: センサ異常 (土壌センサ断線模倣) */
} DEMO_SCENARIO;

typedef struct {
    int16_t leafTemperatureOffsetCentiC;    /* 葉温に加算するオフセット [1/100 ℃] */
    int16_t soilMoistureOffsetPermille;     /* 土壌水分に加算するオフセット [‰] */
    int32_t illuminanceScalePercent;        /* 照度に掛ける倍率[%] (100 = 1.0x) */
    bool forceTankEmpty;                    /* タンク空の強制模倣 */
    bool forceSoilSensorError;              /* 土壌センサ異常の強制模倣 */
} DEMO_MODE_OFFSETS;

typedef struct {
    bool active;                            /* デモモード動作中フラグ */
    DEMO_SCENARIO scenario;                 /* 現在のデモシナリオ */
    DEMO_MODE_OFFSETS offsets;              /* 現在適用中のオフセット */
    uint16_t sw1HoldTicks;                  /* SW1押下継続Tickカウンタ */
    bool sw1LongPressTriggered;             /* 長押し発火済みフラグ */
} DEMO_MODE_STATE;

void DemoMode_Reset(DEMO_MODE_STATE *state);
void DemoMode_ProcessSwitch1Tick(DEMO_MODE_STATE *state, bool sw1Pressed, uint16_t holdThresholdTicks);
bool DemoMode_IsActive(const DEMO_MODE_STATE *state);
void DemoMode_SetActive(DEMO_MODE_STATE *state, bool active);
void DemoMode_SetScenario(DEMO_MODE_STATE *state, DEMO_SCENARIO scenario);
DEMO_SCENARIO DemoMode_GetScenario(const DEMO_MODE_STATE *state);
void DemoMode_ApplyOffsets(const DEMO_MODE_STATE *state,
                           int16_t *leafTempCentiC,
                           int16_t *soilMoisturePermille,
                           int32_t *illuminanceRaw,
                           bool *tankLiquid,
                           SENSOR_HEALTH *soilHealth);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_MODE_H */
