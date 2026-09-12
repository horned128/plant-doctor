/** =================================================================*
 * @file   test_plant_log_queue.c
 * @brief  ログ待機キュー単体テスト (P2-6)
 * ================================================================= */
#include "assert_helper.h"
#include "../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogQueue.h"
#include <string.h>

static void test_queue_init_and_empty(void) {
    PLANT_LOG_QUEUE q;
    PLANT_LOG_RECORD rec;

    PlantLogQueue_Init(&q);
    TEST_ASSERT_TRUE(PlantLogQueue_IsEmpty(&q));
    TEST_ASSERT_FALSE(PlantLogQueue_IsFull(&q));
    TEST_ASSERT_EQUAL_UINT(0U, PlantLogQueue_GetCount(&q));
    TEST_ASSERT_FALSE(PlantLogQueue_Dequeue(&q, &rec));
}

static void test_queue_fifo_order(void) {
    PLANT_LOG_QUEUE q;
    PLANT_LOG_RECORD rec1, rec2, out;

    PlantLogQueue_Init(&q);
    memset(&rec1, 0, sizeof(rec1));
    memset(&rec2, 0, sizeof(rec2));

    rec1.recordSequence = 100U;
    rec1.recordType = 0U; /* 定期 */
    rec2.recordSequence = 101U;
    rec2.recordType = 1U; /* 給水 */

    TEST_ASSERT_TRUE(PlantLogQueue_Enqueue(&q, &rec1));
    TEST_ASSERT_TRUE(PlantLogQueue_Enqueue(&q, &rec2));
    TEST_ASSERT_EQUAL_UINT(2U, PlantLogQueue_GetCount(&q));

    TEST_ASSERT_TRUE(PlantLogQueue_Dequeue(&q, &out));
    TEST_ASSERT_EQUAL_UINT(100U, out.recordSequence);

    TEST_ASSERT_TRUE(PlantLogQueue_Dequeue(&q, &out));
    TEST_ASSERT_EQUAL_UINT(101U, out.recordSequence);

    TEST_ASSERT_TRUE(PlantLogQueue_IsEmpty(&q));
}

static void test_queue_periodic_overflow(void) {
    PLANT_LOG_QUEUE q;
    PLANT_LOG_RECORD rec;
    uint8_t i;

    PlantLogQueue_Init(&q);
    memset(&rec, 0, sizeof(rec));
    rec.recordType = 0U; /* 定期 */

    /* 8個投入して満杯にする */
    for (i = 0U; i < PLANT_DOCTOR_LOG_QUEUE_SIZE; ++i) {
        rec.recordSequence = (uint16_t)(i + 1U);
        TEST_ASSERT_TRUE(PlantLogQueue_Enqueue(&q, &rec));
    }
    TEST_ASSERT_TRUE(PlantLogQueue_IsFull(&q));

    /* 9個目の定期レコード投入 -> 破棄されるはず */
    rec.recordSequence = 999U;
    TEST_ASSERT_FALSE(PlantLogQueue_Enqueue(&q, &rec));
    TEST_ASSERT_EQUAL_UINT(1U, q.droppedPeriodicCount);
    TEST_ASSERT_EQUAL_UINT(PLANT_DOCTOR_LOG_QUEUE_SIZE, PlantLogQueue_GetCount(&q));
}

static void test_queue_event_overwrites_oldest_periodic(void) {
    PLANT_LOG_QUEUE q;
    PLANT_LOG_RECORD rec, out;
    uint8_t i;

    PlantLogQueue_Init(&q);
    memset(&rec, 0, sizeof(rec));
    rec.recordType = 0U; /* 定期 */

    /* 8個の定期レコードを投入 */
    for (i = 0U; i < PLANT_DOCTOR_LOG_QUEUE_SIZE; ++i) {
        rec.recordSequence = (uint16_t)(i + 1U);
        PlantLogQueue_Enqueue(&q, &rec);
    }

    /* イベントレコード (type=1) を投入 -> 最古の定期レコードが置換され、成功する */
    rec.recordSequence = 500U;
    rec.recordType = 1U; /* 給水イベント */
    TEST_ASSERT_TRUE(PlantLogQueue_Enqueue(&q, &rec));
    TEST_ASSERT_EQUAL_UINT(1U, q.droppedPeriodicCount);
    TEST_ASSERT_EQUAL_UINT(0U, q.droppedEventCount);

    /* デキューして先頭がイベントになっていることを確認 */
    TEST_ASSERT_TRUE(PlantLogQueue_Dequeue(&q, &out));
    TEST_ASSERT_EQUAL_UINT(500U, out.recordSequence);
    TEST_ASSERT_EQUAL_UINT(1U, out.recordType);
}

static void test_queue_all_events_overflow(void) {
    PLANT_LOG_QUEUE q;
    PLANT_LOG_RECORD rec, out;
    uint8_t i;

    PlantLogQueue_Init(&q);
    memset(&rec, 0, sizeof(rec));
    rec.recordType = 2U; /* 起動イベント */

    /* 8個のイベントレコードを投入 */
    for (i = 0U; i < PLANT_DOCTOR_LOG_QUEUE_SIZE; ++i) {
        rec.recordSequence = (uint16_t)(i + 1U);
        PlantLogQueue_Enqueue(&q, &rec);
    }

    /* 9個目のイベントレコード投入 -> 最古のイベントが押し出されて破棄カウント増加 */
    rec.recordSequence = 100U;
    rec.recordType = 3U; /* エラーイベント */
    TEST_ASSERT_TRUE(PlantLogQueue_Enqueue(&q, &rec));
    TEST_ASSERT_EQUAL_UINT(1U, q.droppedEventCount);

    /* デキュー: 最古の1Uは破棄されたので2Uから読めるはず */
    TEST_ASSERT_TRUE(PlantLogQueue_Dequeue(&q, &out));
    TEST_ASSERT_EQUAL_UINT(2U, out.recordSequence);
}

int main(void) {
    TEST_RUN(test_queue_init_and_empty);
    TEST_RUN(test_queue_fifo_order);
    TEST_RUN(test_queue_periodic_overflow);
    TEST_RUN(test_queue_event_overwrites_oldest_periodic);
    TEST_RUN(test_queue_all_events_overflow);

    TEST_REPORT_AND_EXIT();
}
