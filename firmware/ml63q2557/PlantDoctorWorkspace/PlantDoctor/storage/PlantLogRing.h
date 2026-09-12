/** =================================================================*
 * @file   PlantLogRing.h
 * @brief  FeRAM 植物ログ永続化リングバッファ (P2-3)
 * ================================================================= */
#ifndef PLANT_LOG_RING_H
#define PLANT_LOG_RING_H

#include <stdint.h>
#include <stdbool.h>
#include "PlantLogRecord.h"

#define PLANT_LOG_RING_MAGIC               (0x474F4C50UL) /* 'PLOG' (Little Endian) */
#define PLANT_LOG_RING_VERSION             (1U)
#define PLANT_LOG_RING_HEADER_ADDR         (0x00000000UL)
#define PLANT_LOG_RING_DATA_ADDR           (0x00000100UL)
#define PLANT_LOG_RING_DATA_SIZE           (261888UL)    /* 256KiB - 256B = 261,888B */
#define PLANT_LOG_RING_CAPACITY            (10912U)      /* 261,888 / 24 = 10,912 records */

/**
 * @brief FeRAM リングバッファヘッダ構造体 (32 bytes)
 */
typedef struct {
    uint32_t magic;                    /**< マジックナンバー ('PLOG') */
    uint16_t version;                  /**< ヘッダバージョン */
    uint16_t recordSize;               /**< 1レコード長 (24) */
    uint32_t capacity;                 /**< 最大収容レコード数 (10912) */
    uint32_t headIndex;                /**< 次回書き込みインデックス (0..10911) */
    uint32_t count;                    /**< 有効レコード数 (0..10912) */
    uint32_t nextSequence;             /**< 次回発番シーケンス番号 */
    uint32_t headerCrc;                /**< ヘッダ整合性CRC32 */
} PLANT_LOG_RING_HEADER;

/**
 * @brief リングバッファの初期化（ヘッダ検証・フォーマット）
 */
void PlantLogRing_Init(void);

/**
 * @brief ログレコードの追加書き込み
 * @return 書き込みに成功した場合 true
 */
bool PlantLogRing_Append(const PLANT_LOG_RECORD *record);

/**
 * @brief 保存済み有効レコード数の取得
 */
uint32_t PlantLogRing_GetCount(void);

/**
 * @brief 最古レコードからの相対インデックスによるレコード読み出し
 * @param indexFromOldest 0: 最古, count-1: 最新
 * @param record 読み出し先構造体ポインタ
 * @return 読み出しおよびCRC検証に成功した場合 true
 */
bool PlantLogRing_ReadRecord(uint32_t indexFromOldest, PLANT_LOG_RECORD *record);

/**
 * @brief リングバッファの消去（ヘッダ再初期化）
 */
void PlantLogRing_Clear(void);

#endif /* PLANT_LOG_RING_H */
