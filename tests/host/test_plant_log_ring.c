/** =================================================================*
 * @file   test_plant_log_ring.c
 * @brief  FeRAM 植物ログリングバッファ単体テスト (P2-3)
 * ================================================================= */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "PlantLogRing.h"
#include "PlantLogRecord.h"

static void createSampleRecord(PLANT_LOG_RECORD *rec, uint16_t seq, uint32_t timestamp) {
    PlantLogRecord_Clear(rec);
    rec->recordSequence = seq;
    rec->timestampSeconds = timestamp;
    rec->recordType = 0U;
    rec->bootCount = 1U;
    rec->plantStatus = 0U;
    rec->stressScore = 20U;
    rec->validFlags = 0x3FU;
    rec->soilMoistureRaw = 2000U;
    rec->leafTemperatureCentiC = 2500;
    rec->airTemperatureCentiC = 2400;
    rec->relativeHumidityCentiPercent = 6000U;
    rec->illuminanceRaw = 1000U;
    rec->eventPayload = 0U;
}

static void test_init_and_clear(void) {
    PlantLogRing_Clear();
    assert(PlantLogRing_GetCount() == 0U);
    printf("PASS: test_init_and_clear\n");
}

static void test_append_and_read(void) {
    PlantLogRing_Clear();

    PLANT_LOG_RECORD recIn, recOut;
    createSampleRecord(&recIn, 101U, 1726000000UL);

    bool appendOk = PlantLogRing_Append(&recIn);
    assert(appendOk);
    assert(PlantLogRing_GetCount() == 1U);

    bool readOk = PlantLogRing_ReadRecord(0U, &recOut);
    assert(readOk);
    assert(recOut.recordSequence == 101U);
    assert(recOut.timestampSeconds == 1726000000UL);
    assert(recOut.soilMoistureRaw == 2000U);
    assert(recOut.leafTemperatureCentiC == 2500);

    /* 範囲外インデックスは失敗すること */
    assert(!PlantLogRing_ReadRecord(1U, &recOut));

    printf("PASS: test_append_and_read\n");
}

static void test_fifo_ordering(void) {
    PlantLogRing_Clear();

    for (uint16_t i = 0U; i < 5U; i++) {
        PLANT_LOG_RECORD rec;
        createSampleRecord(&rec, 1000U + i, 1726000000UL + (uint32_t)i * 60UL);
        assert(PlantLogRing_Append(&rec));
    }
    assert(PlantLogRing_GetCount() == 5U);

    for (uint32_t i = 0U; i < 5U; i++) {
        PLANT_LOG_RECORD out;
        assert(PlantLogRing_ReadRecord(i, &out));
        assert(out.recordSequence == (1000U + (uint16_t)i));
        assert(out.timestampSeconds == (1726000000UL + i * 60UL));
    }

    printf("PASS: test_fifo_ordering\n");
}

static void test_persistence_across_reboot(void) {
    PlantLogRing_Clear();

    for (uint16_t i = 0U; i < 3U; i++) {
        PLANT_LOG_RECORD rec;
        createSampleRecord(&rec, 50U + i, 1726100000UL + (uint32_t)i);
        assert(PlantLogRing_Append(&rec));
    }
    assert(PlantLogRing_GetCount() == 3U);

    /* 再起動（再Init）シミュレーション */
    PlantLogRing_Init();
    assert(PlantLogRing_GetCount() == 3U);

    PLANT_LOG_RECORD out;
    assert(PlantLogRing_ReadRecord(0U, &out));
    assert(out.recordSequence == 50U);
    assert(PlantLogRing_ReadRecord(2U, &out));
    assert(out.recordSequence == 52U);

    printf("PASS: test_persistence_across_reboot\n");
}

int main(void) {
    printf("=== test_plant_log_ring ===\n");
    test_init_and_clear();
    test_append_and_read();
    test_fifo_ordering();
    test_persistence_across_reboot();
    printf("All test_plant_log_ring passed.\n");
    return 0;
}
