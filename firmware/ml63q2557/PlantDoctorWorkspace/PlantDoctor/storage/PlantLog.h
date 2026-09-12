/** =================================================================*
 * @file   PlantLog.h
 * @brief  植物ログ保存API
 * ================================================================= */
#ifndef PLANT_LOG_H
#define PLANT_LOG_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "PlantLogRecord.h"                                 /* ログレコード型定義 */
#include "PlantDoctorStatus.h"                              /* ステータス型定義 */
#include "PumpControl.h"                                    /* 給水イベント型定義 */

bool PlantLog_Init(void);                                   /* PlantLog_InitのAPI */
bool PlantLog_Append(const uint8_t *data, uint16_t size);   /* PlantLog_AppendのAPI */
void PlantLog_Process10Ms(void);                            /* PlantLog_Process10MsのAPI */
uint16_t PlantLog_GetRecordCount(void);                     /* 保持レコード数取得 */
bool PlantLog_ReadRecord(uint16_t index, PLANT_LOG_RECORD *record); /* index番目のレコード取得 */
bool PlantLog_Erase(void);                                  /* 記録全消去 */
void PlantLog_NotifyWatering(const PUMP_WATERING_EVENT *event); /* 給水イベント記録要求 */
void PlantLog_NotifyError(PLANT_DOCTOR_ERROR error);         /* エラー記録要求 */
uint8_t PlantLog_GetBootCount(void);                        /* 起動回数取得 */

#endif /* PLANT_LOG_H */
