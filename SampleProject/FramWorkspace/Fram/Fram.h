/*****************************************************************************
 * File: Fram.h
 * Title: FRAMモジュールのインターフェース
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Fram.h
 * @brief FRAMモジュールのインターフェース
 * @note 本モジュールの複数バイトの読み書きはCPUのエンディアンで行う。
 */

#ifndef FRAM_H__
#define FRAM_H__

#include <stdint.h>
#include <stdbool.h>
/**
 * @brief MB85RS2MTAの初期化を行う。
 * 
 * @note 通信を行うため、SoftSpiが動作するよう初期化を完了させておく必要がある。
 */
void FramInit(void);

/**
 * @brief Framから1バイト読み込む。
 * 
 * @param address 読込対象となるFramのアドレス
 * @param byte 読込結果の格納先
 */
void FramReadByte(uint32_t address, uint8_t* byte);

/**
 * @brief Framから2バイト読み込む。
 * 
 * @param address 読込対象となるFramのアドレス
 * @param halfWord 読込結果の格納先
 */
void FramReadHalfWord(uint32_t address, uint16_t* halfWord);

/**
 * @brief Framから4バイト読み込む。
 * 
 * @param address 読込対象となるFramのアドレス
 * @param word 読込結果の格納先
 */
void FramReadWord(uint32_t address, uint32_t* word);

/**
 * @brief Framから指定バイト数読み込む。
 * 
 * @param address 読込対象となるFramのアドレス
 * @param dst 読込結果の格納先
 * @param size 読込バイト数
 */
void FramReadBlock(uint32_t address, void* dst, int size);

/**
 * @brief Framに1バイト書き込む。
 * 
 * @param address 書込対象となるFramのアドレス
 * @param byte 書込むデータ
 */
void FramWriteByte(uint32_t address, uint8_t byte);

/**
 * @brief Framに2バイト書き込む。
 * 
 * @param address 書込対象となるFramのアドレス
 * @param halfWord 書込むデータ
 */
void FramWriteHalfWord(uint32_t address, uint16_t halfWord);

/**
 * @brief Framに4バイト書き込む。
 * 
 * @param address 書込対象となるFramのアドレス
 * @param word 書込むデータ
 */
void FramWriteWord(uint32_t address, uint32_t word);

/**
 * @brief Framに指定バイト数書き込む。
 * 
 * @param address 書込対象となるFramのアドレス
 * @param src 書込むデータ群の先頭を示すポインタ
 * @param size 書込バイト数
 */
void FramWriteBlock(uint32_t address, const void* src, int size);
#endif // FRAM_H__
