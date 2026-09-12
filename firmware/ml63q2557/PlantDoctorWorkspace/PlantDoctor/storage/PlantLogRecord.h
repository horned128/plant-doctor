/** =================================================================*
 * @file   PlantLogRecord.h
 * @brief  記録レコード形式とエンコーダ／デコーダAPI
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef PLANT_LOG_RECORD_H
#define PLANT_LOG_RECORD_H
#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"              /* 状態定義 */

#define PLANT_LOG_RECORD_SIZE                  (24U)

typedef struct {
    uint32_t timestampSeconds;                              /* RTC由来UNIX秒(未同期:0xFFFFFFFF) */
    uint16_t recordSequence;                                /* 記録レコード連番 */
    uint8_t recordType;                                     /* 0:定期, 1:給水, 2:起動, 3:エラー, 4:診断変化 */
    uint8_t validFlags;                                     /* bit0 soil, 1 leaf, 2 air+hum, 3 press, 4 lux, 5 tank */
    uint16_t soilMoistureRaw;                               /* 0〜4095 */
    int16_t leafTemperatureCentiC;                          /* 摂氏1/100 */
    int16_t airTemperatureCentiC;                           /* 摂氏1/100 */
    uint16_t relativeHumidityCentiPercent;                  /* %RHの1/100 */
    uint16_t illuminanceRaw;                                /* 照度生値 */
    uint8_t plantStatus;                                    /* PLANT_STATUS */
    uint8_t stressScore;                                    /* 0〜100、不明時は0xFF */
    uint16_t eventPayload;                                  /* 給水時間[10ms]等 */
    uint8_t bootCount;                                      /* 再起動判別用 */
    uint8_t crc8;                                           /* CRC-8 (offset 0〜22) */
} PLANT_LOG_RECORD;

void PlantLogRecord_Clear(PLANT_LOG_RECORD *record);        /* レコード初期化 */
void PlantLogRecord_Encode(const PLANT_LOG_RECORD *record, uint8_t *buffer); /* 24 byteへ符号化 */
bool PlantLogRecord_Decode(const uint8_t *buffer, PLANT_LOG_RECORD *record); /* CRC検証付き復号 */
uint8_t PlantLogRecord_Crc8(const uint8_t *data, uint8_t size); /* CRC-8算出 */
uint8_t PlantLogRecord_FormatCsv(const PLANT_LOG_RECORD *record, char *text, uint8_t textSize); /* CSV整形 */

#endif /* PLANT_LOG_RECORD_H */
