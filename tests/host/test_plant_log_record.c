/** =================================================================*
 * @file   test_plant_log_record.c
 * @brief  記録レコード形式・CRC・符号化単体テスト
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"                  /* ホストテスト用アサーションマクロ */
#include "PlantLogRecord.h"                 /* 記録レコードAPI */

/** =================================================================*
 * @brief  符号化と復号のラウンドトリップ検証
 * ================================================================= */
static void Test_EncodeDecodeRoundTrip(void) {
    PLANT_LOG_RECORD original;
    PLANT_LOG_RECORD decoded;
    uint8_t buffer[PLANT_LOG_RECORD_SIZE];

    PlantLogRecord_Clear(&original);
    original.timestampSeconds = 1773300000UL;
    original.recordSequence = 1234U;
    original.recordType = 1U;
    original.validFlags = 0x3FU;
    original.soilMoistureRaw = 2500U;
    original.leafTemperatureCentiC = -500;   /* -5.00 ℃ */
    original.airTemperatureCentiC = -250;    /* -2.50 ℃ */
    original.relativeHumidityCentiPercent = 4500U;
    original.illuminanceRaw = 1200U;
    original.plantStatus = (uint8_t)PLANT_STATUS_DRY_STRESS;
    original.stressScore = 75U;
    original.eventPayload = 200U;
    original.bootCount = 3U;

    PlantLogRecord_Encode(&original, buffer);
    TEST_ASSERT_TRUE(PlantLogRecord_Decode(buffer, &decoded));

    TEST_ASSERT_EQUAL_UINT(original.timestampSeconds, decoded.timestampSeconds);
    TEST_ASSERT_EQUAL_UINT(original.recordSequence, decoded.recordSequence);
    TEST_ASSERT_EQUAL_UINT(original.recordType, decoded.recordType);
    TEST_ASSERT_EQUAL_UINT(original.validFlags, decoded.validFlags);
    TEST_ASSERT_EQUAL_UINT(original.soilMoistureRaw, decoded.soilMoistureRaw);
    TEST_ASSERT_EQUAL_INT(original.leafTemperatureCentiC, decoded.leafTemperatureCentiC);
    TEST_ASSERT_EQUAL_INT(original.airTemperatureCentiC, decoded.airTemperatureCentiC);
    TEST_ASSERT_EQUAL_UINT(original.relativeHumidityCentiPercent, decoded.relativeHumidityCentiPercent);
    TEST_ASSERT_EQUAL_UINT(original.illuminanceRaw, decoded.illuminanceRaw);
    TEST_ASSERT_EQUAL_UINT(original.plantStatus, decoded.plantStatus);
    TEST_ASSERT_EQUAL_UINT(original.stressScore, decoded.stressScore);
    TEST_ASSERT_EQUAL_UINT(original.eventPayload, decoded.eventPayload);
    TEST_ASSERT_EQUAL_UINT(original.bootCount, decoded.bootCount);
    TEST_ASSERT_EQUAL_UINT(buffer[23], decoded.crc8);
}

/** =================================================================*
 * @brief  CRC不一致の検出検証
 * ================================================================= */
static void Test_CrcErrorDetection(void) {
    PLANT_LOG_RECORD record;
    PLANT_LOG_RECORD decoded;
    uint8_t buffer[PLANT_LOG_RECORD_SIZE];

    PlantLogRecord_Clear(&record);
    record.timestampSeconds = 1000UL;
    record.recordSequence = 1U;

    PlantLogRecord_Encode(&record, buffer);

    /* データ部を1バイト改ざん */
    buffer[8] ^= 0x01U;
    TEST_ASSERT_FALSE(PlantLogRecord_Decode(buffer, &decoded));

    /* 元に戻してCRC自身を改ざん */
    buffer[8] ^= 0x01U;
    buffer[23] ^= 0x55U;
    TEST_ASSERT_FALSE(PlantLogRecord_Decode(buffer, &decoded));
}

/** =================================================================*
 * @brief  期待バイト列とのバイト単位完全一致検証
 * ================================================================= */
static void Test_ExactByteComparison(void) {
    PLANT_LOG_RECORD record;
    uint8_t buffer[PLANT_LOG_RECORD_SIZE];
    uint8_t expected[PLANT_LOG_RECORD_SIZE];
    int i;

    PlantLogRecord_Clear(&record);
    record.timestampSeconds = 0x12345678UL;
    record.recordSequence = 0x0201U;
    record.recordType = 0x03U;
    record.validFlags = 0x04U;
    record.soilMoistureRaw = 0x0605U;
    record.leafTemperatureCentiC = (int16_t)0x0807;
    record.airTemperatureCentiC = (int16_t)0x0A09;
    record.relativeHumidityCentiPercent = 0x0C0BU;
    record.illuminanceRaw = 0x0E0DU;
    record.plantStatus = 0x0FU;
    record.stressScore = 0x10U;
    record.eventPayload = 0x1211U;
    record.bootCount = 0x13U;

    expected[0] = 0x78U;
    expected[1] = 0x56U;
    expected[2] = 0x34U;
    expected[3] = 0x12U;
    expected[4] = 0x01U;
    expected[5] = 0x02U;
    expected[6] = 0x03U;
    expected[7] = 0x04U;
    expected[8] = 0x05U;
    expected[9] = 0x06U;
    expected[10] = 0x07U;
    expected[11] = 0x08U;
    expected[12] = 0x09U;
    expected[13] = 0x0AU;
    expected[14] = 0x0BU;
    expected[15] = 0x0CU;
    expected[16] = 0x0DU;
    expected[17] = 0x0EU;
    expected[18] = 0x0FU;
    expected[19] = 0x10U;
    expected[20] = 0x11U;
    expected[21] = 0x12U;
    expected[22] = 0x13U;
    expected[23] = PlantLogRecord_Crc8(expected, 23U);

    PlantLogRecord_Encode(&record, buffer);

    for (i = 0; i < (int)PLANT_LOG_RECORD_SIZE; ++i) {
        TEST_ASSERT_EQUAL_UINT(expected[i], buffer[i]);
    }
}

/** =================================================================*
 * @brief  連番ラップアラウンド時のモジュロ差分検証
 * ================================================================= */
static void Test_RecordSequenceModulo(void) {
    uint16_t prev = 65535U;
    uint16_t curr = 0U;
    uint16_t diff = (uint16_t)(curr - prev);

    TEST_ASSERT_EQUAL_UINT(1U, diff);
}

/** =================================================================*
 * @brief  CSV整形の検証
 * ================================================================= */
static void Test_CsvFormatting(void) {
    PLANT_LOG_RECORD record;
    char text[128];
    char shortText[10];
    uint8_t len;

    PlantLogRecord_Clear(&record);
    record.timestampSeconds = 1000UL;
    record.recordSequence = 1U;
    record.recordType = 0U;
    record.validFlags = 1U;
    record.soilMoistureRaw = 2000U;
    record.leafTemperatureCentiC = 2500;
    record.airTemperatureCentiC = 2400;
    record.relativeHumidityCentiPercent = 5000U;
    record.illuminanceRaw = 300U;
    record.plantStatus = 0U;
    record.stressScore = 15U;
    record.eventPayload = 0U;
    record.bootCount = 1U;

    len = PlantLogRecord_FormatCsv(&record, text, sizeof(text));
    TEST_ASSERT_TRUE(len > 0U);
    TEST_ASSERT_EQUAL_STRING("1000,1,0,1,2000,2500,2400,5000,300,0,15,0,1\r\n", text);

    /* バッファ長不足時は0を返す */
    len = PlantLogRecord_FormatCsv(&record, shortText, sizeof(shortText));
    TEST_ASSERT_EQUAL_UINT(0U, len);
}

/** =================================================================*
 * @brief  テストメインエントリ
 * @return 成功時0、失敗時1
 * ================================================================= */
int main(void) {
    TEST_RUN(Test_EncodeDecodeRoundTrip);
    TEST_RUN(Test_CrcErrorDetection);
    TEST_RUN(Test_ExactByteComparison);
    TEST_RUN(Test_RecordSequenceModulo);
    TEST_RUN(Test_CsvFormatting);

    TEST_REPORT_AND_EXIT();
}
