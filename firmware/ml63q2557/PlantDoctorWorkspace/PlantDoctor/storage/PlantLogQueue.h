/** =================================================================*
 * @file   PlantLogQueue.h
 * @brief  ログ記録待機キューAPI (P2-6)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef PLANT_LOG_QUEUE_H
#define PLANT_LOG_QUEUE_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantLogRecord.h"                 /* レコード型定義 */
#include "PlantDoctorConfig.h"              /* 設定マクロ */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PLANT_LOG_RECORD entries[PLANT_DOCTOR_LOG_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    uint32_t droppedPeriodicCount;          /* 破棄された定期レコード数 */
    uint32_t droppedEventCount;             /* 破棄されたイベントレコード数 */
} PLANT_LOG_QUEUE;

void PlantLogQueue_Init(PLANT_LOG_QUEUE *queue);
bool PlantLogQueue_Enqueue(PLANT_LOG_QUEUE *queue, const PLANT_LOG_RECORD *record);
bool PlantLogQueue_Dequeue(PLANT_LOG_QUEUE *queue, PLANT_LOG_RECORD *record);
uint8_t PlantLogQueue_GetCount(const PLANT_LOG_QUEUE *queue);
bool PlantLogQueue_IsEmpty(const PLANT_LOG_QUEUE *queue);
bool PlantLogQueue_IsFull(const PLANT_LOG_QUEUE *queue);

#ifdef __cplusplus
}
#endif

#endif /* PLANT_LOG_QUEUE_H */
