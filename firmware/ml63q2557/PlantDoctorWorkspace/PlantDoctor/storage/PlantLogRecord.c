/** =================================================================*
 * @file   PlantLogRecord.c
 * @brief  記録レコード形式とエンコーダ／デコーダ
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "PlantLogRecord.h"                                 /* レコード形式API */
#include <stdio.h>                                          /* snprintf */
#include <string.h>                                         /* memset */

/** =================================================================*
 * @brief  レコード構造体の初期化
 * @param[out] record 初期化対象レコード
 * ================================================================= */
void PlantLogRecord_Clear(PLANT_LOG_RECORD *record) {
    if (record != 0) {
        (void)memset(record, 0, sizeof(*record));
        record->timestampSeconds = 0xFFFFFFFFUL;
        record->stressScore = PLANT_STRESS_UNKNOWN;
    }
}

/** =================================================================*
 * @brief  CRC-8算出（多項式0x07、初期値0x00）
 * @param[in] data 対象データ
 * @param[in] size 対象サイズ
 * @return CRC-8値
 * ================================================================= */
uint8_t PlantLogRecord_Crc8(const uint8_t *data, uint8_t size) {
    uint8_t crc = 0x00U;
    uint8_t i;
    uint8_t b;

    if (data == 0) {
        return 0U;
    }

    for (i = 0U; i < size; ++i) {
        crc ^= data[i];
        for (b = 0U; b < 8U; ++b) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1U) ^ 0x07U);
            } else {
                crc = (uint8_t)(crc << 1U);
            }
        }
    }

    return crc;
}

/** =================================================================*
 * @brief  レコードを24バイト固定長へ符号化（リトルエンディアン）
 * @param[in]  record レコード入力
 * @param[out] buffer 出力バッファ（24バイト以上）
 * ================================================================= */
void PlantLogRecord_Encode(const PLANT_LOG_RECORD *record, uint8_t *buffer) {
    uint16_t u16;

    if ((record == 0) || (buffer == 0)) {
        return;
    }

    /* offset 0..3: timestampSeconds */
    buffer[0] = (uint8_t)(record->timestampSeconds & 0xFFU);
    buffer[1] = (uint8_t)((record->timestampSeconds >> 8U) & 0xFFU);
    buffer[2] = (uint8_t)((record->timestampSeconds >> 16U) & 0xFFU);
    buffer[3] = (uint8_t)((record->timestampSeconds >> 24U) & 0xFFU);

    /* offset 4..5: recordSequence */
    buffer[4] = (uint8_t)(record->recordSequence & 0xFFU);
    buffer[5] = (uint8_t)((record->recordSequence >> 8U) & 0xFFU);

    /* offset 6: recordType */
    buffer[6] = record->recordType;

    /* offset 7: validFlags */
    buffer[7] = record->validFlags;

    /* offset 8..9: soilMoistureRaw */
    buffer[8] = (uint8_t)(record->soilMoistureRaw & 0xFFU);
    buffer[9] = (uint8_t)((record->soilMoistureRaw >> 8U) & 0xFFU);

    /* offset 10..11: leafTemperatureCentiC */
    u16 = (uint16_t)record->leafTemperatureCentiC;
    buffer[10] = (uint8_t)(u16 & 0xFFU);
    buffer[11] = (uint8_t)((u16 >> 8U) & 0xFFU);

    /* offset 12..13: airTemperatureCentiC */
    u16 = (uint16_t)record->airTemperatureCentiC;
    buffer[12] = (uint8_t)(u16 & 0xFFU);
    buffer[13] = (uint8_t)((u16 >> 8U) & 0xFFU);

    /* offset 14..15: relativeHumidityCentiPercent */
    buffer[14] = (uint8_t)(record->relativeHumidityCentiPercent & 0xFFU);
    buffer[15] = (uint8_t)((record->relativeHumidityCentiPercent >> 8U) & 0xFFU);

    /* offset 16..17: illuminanceRaw */
    buffer[16] = (uint8_t)(record->illuminanceRaw & 0xFFU);
    buffer[17] = (uint8_t)((record->illuminanceRaw >> 8U) & 0xFFU);

    /* offset 18: plantStatus */
    buffer[18] = record->plantStatus;

    /* offset 19: stressScore */
    buffer[19] = record->stressScore;

    /* offset 20..21: eventPayload */
    buffer[20] = (uint8_t)(record->eventPayload & 0xFFU);
    buffer[21] = (uint8_t)((record->eventPayload >> 8U) & 0xFFU);

    /* offset 22: bootCount */
    buffer[22] = record->bootCount;

    /* offset 23: crc8 */
    buffer[23] = PlantLogRecord_Crc8(buffer, 23U);
}

/** =================================================================*
 * @brief  24バイト固定長からレコード復号（CRC検証付き）
 * @param[in]  buffer 入力バッファ（24バイト）
 * @param[out] record 復号先レコード
 * @return 正常に復号できCRCが一致した場合true、異常時false
 * ================================================================= */
bool PlantLogRecord_Decode(const uint8_t *buffer, PLANT_LOG_RECORD *record) {
    uint8_t expectedCrc;
    uint16_t u16;

    if ((buffer == 0) || (record == 0)) {
        return false;
    }

    expectedCrc = PlantLogRecord_Crc8(buffer, 23U);
    if (buffer[23] != expectedCrc) {
        return false;
    }

    record->timestampSeconds = ((uint32_t)buffer[0]) |
        (((uint32_t)buffer[1]) << 8U) |
        (((uint32_t)buffer[2]) << 16U) |
        (((uint32_t)buffer[3]) << 24U);

    record->recordSequence = (uint16_t)(((uint16_t)buffer[4]) | (((uint16_t)buffer[5]) << 8U));
    record->recordType = buffer[6];
    record->validFlags = buffer[7];
    record->soilMoistureRaw = (uint16_t)(((uint16_t)buffer[8]) | (((uint16_t)buffer[9]) << 8U));

    u16 = (uint16_t)(((uint16_t)buffer[10]) | (((uint16_t)buffer[11]) << 8U));
    record->leafTemperatureCentiC = (int16_t)u16;

    u16 = (uint16_t)(((uint16_t)buffer[12]) | (((uint16_t)buffer[13]) << 8U));
    record->airTemperatureCentiC = (int16_t)u16;

    record->relativeHumidityCentiPercent = (uint16_t)(((uint16_t)buffer[14]) | (((uint16_t)buffer[15]) << 8U));
    record->illuminanceRaw = (uint16_t)(((uint16_t)buffer[16]) | (((uint16_t)buffer[17]) << 8U));
    record->plantStatus = buffer[18];
    record->stressScore = buffer[19];
    record->eventPayload = (uint16_t)(((uint16_t)buffer[20]) | (((uint16_t)buffer[21]) << 8U));
    record->bootCount = buffer[22];
    record->crc8 = buffer[23];

    return true;
}

/** =================================================================*
 * @brief  レコードをCSV1行へ整形
 * @param[in]  record レコード入力
 * @param[out] text   出力文字列バッファ
 * @param[in]  textSize 出力文字列バッファ長
 * @return 書き込まれた文字数（終端NULLを含まない）。バッファ不足時は0
 * ================================================================= */
uint8_t PlantLogRecord_FormatCsv(const PLANT_LOG_RECORD *record, char *text, uint8_t textSize) {
    int written;

    if ((record == 0) || (text == 0) || (textSize == 0U)) {
        return 0U;
    }

    written = snprintf(text, (size_t)textSize,
        "%lu,%u,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u\r\n",
        (unsigned long)record->timestampSeconds,
        (unsigned int)record->recordSequence,
        (unsigned int)record->recordType,
        (unsigned int)record->validFlags,
        (unsigned int)record->soilMoistureRaw,
        (int)record->leafTemperatureCentiC,
        (int)record->airTemperatureCentiC,
        (unsigned int)record->relativeHumidityCentiPercent,
        (unsigned int)record->illuminanceRaw,
        (unsigned int)record->plantStatus,
        (unsigned int)record->stressScore,
        (unsigned int)record->eventPayload,
        (unsigned int)record->bootCount);

    if ((written <= 0) || (written >= (int)textSize)) {
        text[0] = '\0';
        return 0U;
    }

    return (uint8_t)written;
}
