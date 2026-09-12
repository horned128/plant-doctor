/** =================================================================*
 * @file   PumpControl.h
 * @brief  ポンプ制御API
 * ================================================================= */
#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

typedef enum {
    PUMP_CONTROL_STATUS_OK = 0,
    PUMP_CONTROL_STATUS_EMPTY,
    PUMP_CONTROL_STATUS_COOLDOWN,
    PUMP_CONTROL_STATUS_ERROR                               /**< @note 現状未使用（互換性維持のため保持） */
} PUMP_CONTROL_STATUS;

typedef enum {
    PUMP_STOP_REASON_MANUAL = 0,                            /* 手動停止 */
    PUMP_STOP_REASON_MAX_ON_TIME,                           /* 最大駆動時間による自動停止 */
    PUMP_STOP_REASON_EMERGENCY                              /* 緊急停止 */
} PUMP_STOP_REASON;

typedef struct {
    uint32_t startTimeSeconds;                              /* 給水開始時のUNIX秒 */
    uint16_t onDurationTicks;                               /* 実際のON時間 [10 ms] */
    uint16_t soilMoistureRawBefore;                         /* 給水直前の土壌水分生値 */
    int16_t soilMoisturePermilleBefore;                     /* 給水直前の土壌水分千分率 [‰] */
    int16_t leafAirDeltaBefore;                             /* 給水直前の葉温気温差 [1/100 ℃] */
    uint8_t stopReason;                                     /* PUMP_STOP_REASON */
    bool tankLiquidAtStart;                                 /* 給水開始時の液面検出状態 */
    bool automatic;                                         /* 自律給水ならtrue、手動ならfalse */
} PUMP_WATERING_EVENT;

bool PumpControl_Init(void);                                /* PumpControl_InitのAPI */
void PumpControl_Process10Ms(void);                         /* PumpControl_Process10MsのAPI */
PUMP_CONTROL_STATUS PumpControl_Request(bool on);           /* PumpControl_RequestのAPI */
bool PumpControl_IsOn(void);                                /* PumpControl_IsOnのAPI */
void PumpControl_EmergencyStop(void);                       /* 緊急停止API */

bool PumpControl_TakeWateringEvent(PUMP_WATERING_EVENT *event); /* 未取得の給水イベントを取り出す */
void PumpControl_SetPreWateringContext(uint32_t timeSeconds,
                                       uint16_t soilRaw,
                                       int16_t soilPermille,
                                       int16_t leafAirDelta,
                                       bool automatic);     /* 給水前の文脈を設定 */

#endif /* PUMP_CONTROL_H */
