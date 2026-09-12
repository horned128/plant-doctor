/** =================================================================*
 * @file   PlantLogQueue.c
 * @brief  ログ記録待機キューAPI実装 (P2-6)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "PlantLogQueue.h"
#include <string.h>
#include <stddef.h>

/** =================================================================*
 * @brief  PlantLogQueue_Init処理
 * @param[out] queue キュー構造体
 * ================================================================= */
void PlantLogQueue_Init(PLANT_LOG_QUEUE *queue) {
    if (queue == NULL) {
        return;
    }
    (void)memset(queue, 0, sizeof(PLANT_LOG_QUEUE));
}

/** =================================================================*
 * @brief  PlantLogQueue_Enqueue処理
 * @details キュー満杯時、イベントレコード(type != 0)は最古の定期レコード(type == 0)
 *          を破棄して優先格納する。全件イベントの場合は最古のイベントを破棄する。
 *          定期レコードが満杯時に投入された場合は破棄される。
 * @param[in,out] queue キュー構造体
 * @param[in] record 格納対象レコード
 * @return 格納成功時true、破棄時false
 * ================================================================= */
bool PlantLogQueue_Enqueue(PLANT_LOG_QUEUE *queue, const PLANT_LOG_RECORD *record) {
    if ((queue == NULL) || (record == NULL)) {
        return false;
    }

    if (queue->count < PLANT_DOCTOR_LOG_QUEUE_SIZE) {
        queue->entries[queue->head] = *record;
        queue->head = (uint8_t)((queue->head + 1U) % PLANT_DOCTOR_LOG_QUEUE_SIZE);
        ++queue->count;
        return true;
    }

    /* 満杯時の処理 */
    if (record->recordType == 0U) {
        /* 定期レコードは満杯時に破棄 */
        ++queue->droppedPeriodicCount;
        return false;
    }

    /* イベントレコード: 最古の定期レコードを探して置換 */
    {
        uint8_t i;
        int16_t periodicIndex = -1;

        for (i = 0U; i < queue->count; ++i) {
            uint8_t idx = (uint8_t)((queue->tail + i) % PLANT_DOCTOR_LOG_QUEUE_SIZE);
            if (queue->entries[idx].recordType == 0U) {
                periodicIndex = (int16_t)idx;
                break;
            }
        }

        if (periodicIndex >= 0) {
            /* 定期レコードの位置へ上書き */
            queue->entries[periodicIndex] = *record;
            ++queue->droppedPeriodicCount;
            return true;
        }

        /* キュー全体がイベントで満杯の場合: 最古のイベントを1件破棄して追加 */
        queue->tail = (uint8_t)((queue->tail + 1U) % PLANT_DOCTOR_LOG_QUEUE_SIZE);
        queue->entries[queue->head] = *record;
        queue->head = (uint8_t)((queue->head + 1U) % PLANT_DOCTOR_LOG_QUEUE_SIZE);
        ++queue->droppedEventCount;
        return true;
    }
}

/** =================================================================*
 * @brief  PlantLogQueue_Dequeue処理
 * @param[in,out] queue キュー構造体
 * @param[out] record 取り出しレコード出力先
 * @return 取得成功時true、キュー空時false
 * ================================================================= */
bool PlantLogQueue_Dequeue(PLANT_LOG_QUEUE *queue, PLANT_LOG_RECORD *record) {
    if ((queue == NULL) || (record == NULL) || (queue->count == 0U)) {
        return false;
    }

    *record = queue->entries[queue->tail];
    queue->tail = (uint8_t)((queue->tail + 1U) % PLANT_DOCTOR_LOG_QUEUE_SIZE);
    --queue->count;
    return true;
}

/** =================================================================*
 * @brief  PlantLogQueue_GetCount処理
 * ================================================================= */
uint8_t PlantLogQueue_GetCount(const PLANT_LOG_QUEUE *queue) {
    return (queue != NULL) ? queue->count : 0U;
}

/** =================================================================*
 * @brief  PlantLogQueue_IsEmpty処理
 * ================================================================= */
bool PlantLogQueue_IsEmpty(const PLANT_LOG_QUEUE *queue) {
    return (queue == NULL) || (queue->count == 0U);
}

/** =================================================================*
 * @brief  PlantLogQueue_IsFull処理
 * ================================================================= */
bool PlantLogQueue_IsFull(const PLANT_LOG_QUEUE *queue) {
    return (queue != NULL) && (queue->count >= PLANT_DOCTOR_LOG_QUEUE_SIZE);
}
