/** =================================================================*
 * @file   PlantLogRing.c
 * @brief  FeRAM 植物ログ永続化リングバッファ (P2-3)
 * ================================================================= */
#include "PlantLogRing.h"
#include "FramDriver.h"
#include <string.h>

static PLANT_LOG_RING_HEADER s_header;

#include <stddef.h>

static uint32_t computeHeaderCrc(const PLANT_LOG_RING_HEADER *hdr)
{
    /* headerCrc フィールドを除く手前バイトの CRC-32 (IEEE 802.3) 計算 */
    const uint8_t *data = (const uint8_t *)hdr;
    uint32_t crc = 0xFFFFFFFFUL;
    size_t len = offsetof(PLANT_LOG_RING_HEADER, headerCrc);
    for (size_t i = 0U; i < len; i++) {
        crc ^= (uint32_t)data[i];
        for (uint8_t b = 0U; b < 8U; b++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc >>= 1U;
            }
        }
    }
    return ~crc;
}

static void saveHeader(void)
{
    s_header.headerCrc = computeHeaderCrc(&s_header);
    FramDriver_WriteBlock(PLANT_LOG_RING_HEADER_ADDR, &s_header, sizeof(s_header));
}

void PlantLogRing_Init(void)
{
    PLANT_LOG_RING_HEADER loaded;
    FramDriver_ReadBlock(PLANT_LOG_RING_HEADER_ADDR, &loaded, sizeof(loaded));

    if (loaded.magic == PLANT_LOG_RING_MAGIC &&
        loaded.version == PLANT_LOG_RING_VERSION &&
        loaded.recordSize == PLANT_LOG_RECORD_SIZE &&
        loaded.capacity == PLANT_LOG_RING_CAPACITY &&
        loaded.headIndex < PLANT_LOG_RING_CAPACITY &&
        loaded.count <= PLANT_LOG_RING_CAPACITY &&
        loaded.headerCrc == computeHeaderCrc(&loaded)) {
        /* 既存の有効ヘッダを復元 */
        s_header = loaded;
    } else {
        /* ヘッダ初期化 / フォーマット */
        PlantLogRing_Clear();
    }
}

void PlantLogRing_Clear(void)
{
    (void)memset(&s_header, 0, sizeof(s_header));
    s_header.magic = PLANT_LOG_RING_MAGIC;
    s_header.version = PLANT_LOG_RING_VERSION;
    s_header.recordSize = PLANT_LOG_RECORD_SIZE;
    s_header.capacity = PLANT_LOG_RING_CAPACITY;
    s_header.headIndex = 0U;
    s_header.count = 0U;
    s_header.nextSequence = 1U;
    saveHeader();
}

bool PlantLogRing_Append(const PLANT_LOG_RECORD *record)
{
    if (record == (void *)0) {
        return false;
    }

    uint8_t wireBuffer[PLANT_LOG_RECORD_SIZE];
    PlantLogRecord_Encode(record, wireBuffer);

    uint32_t writeAddr = PLANT_LOG_RING_DATA_ADDR + (s_header.headIndex * PLANT_LOG_RECORD_SIZE);
    FramDriver_WriteBlock(writeAddr, wireBuffer, PLANT_LOG_RECORD_SIZE);

    s_header.headIndex = (s_header.headIndex + 1U) % PLANT_LOG_RING_CAPACITY;
    if (s_header.count < PLANT_LOG_RING_CAPACITY) {
        s_header.count++;
    }
    s_header.nextSequence++;
    saveHeader();
    return true;
}

uint32_t PlantLogRing_GetCount(void)
{
    return s_header.count;
}

bool PlantLogRing_ReadRecord(uint32_t indexFromOldest, PLANT_LOG_RECORD *record)
{
    if (record == (void *)0 || indexFromOldest >= s_header.count) {
        return false;
    }

    uint32_t oldestIndex;
    if (s_header.count < PLANT_LOG_RING_CAPACITY) {
        oldestIndex = 0U;
    } else {
        oldestIndex = s_header.headIndex;
    }

    uint32_t targetPhysicalIndex = (oldestIndex + indexFromOldest) % PLANT_LOG_RING_CAPACITY;
    uint32_t readAddr = PLANT_LOG_RING_DATA_ADDR + (targetPhysicalIndex * PLANT_LOG_RECORD_SIZE);

    uint8_t wireBuffer[PLANT_LOG_RECORD_SIZE];
    FramDriver_ReadBlock(readAddr, wireBuffer, sizeof(wireBuffer));

    return PlantLogRecord_Decode(wireBuffer, record);
}
