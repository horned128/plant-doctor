/*****************************************************************************
 * File: Output.h
 * Title: 汎用出力を使う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Output.h
 * @brief 汎用出力を使う。
 */

#ifndef OUTPUT_H__
#define OUTPUT_H__
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 出力の状態を示す列挙型
 */
typedef enum
{
	OUTPUT_STATUS_NULL = 0,	/**< Nullポインタ */
	OUTPUT_STATUS_OFF = 1,	/**< 出力off */
	OUTPUT_STATUS_ON = 2,	/**< 出力on */
}OUTPUT_STATUS;

/**
 * @brief uint8_tの変数の出力をOnにする。
 * 
 * @param dst 出力対象
 * @param value 出力値
 * @return OUTPUT_STATUS 
 */
OUTPUT_STATUS OutputOnUInt8(volatile void* dst, uint8_t value);

/**
 * @brief uint8_tの変数の出力をOffにする。
 * 
 * @param dst 出力対象
 * @param value 出力値
 * @return OUTPUT_STATUS 
 */
OUTPUT_STATUS OutputOffUInt8(volatile void* dst, uint8_t value);

/**
 * @brief uint16_tの変数の出力をOnにする。
 * 
 * @param dst 出力対象
 * @param value 出力値
 * @return OUTPUT_STATUS 
 */
OUTPUT_STATUS OutputOnUInt16(volatile void* dst, uint16_t value);

/**
 * @brief uint16_tの変数の出力をOffにする。
 * 
 * @param dst 出力対象
 * @param value 出力値
 * @return OUTPUT_STATUS 
 */
OUTPUT_STATUS OutputOffUInt16(volatile void* dst, uint16_t value);

/**
 * @brief uint32_tの変数の出力をOnにする。
 * 
 * @param dst 出力対象
 * @param value 出力値
 * @return OUTPUT_STATUS 
 */
OUTPUT_STATUS OutputOnUInt32(volatile void* dst, uint32_t value);

/**
 * @brief uint32_tの変数の出力をOffにする。
 * 
 * @param dst 出力対象
 * @param value 出力値
 * @return OUTPUT_STATUS 
 */
OUTPUT_STATUS OutputOffUInt32(volatile void* dst, uint32_t value);
#endif //OUTPUT_H__
