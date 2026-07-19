/*****************************************************************************
 * File: Rx4111.h
 * Title: RX4111リアルタイムクロックモジュール
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Rx4111.h
 * @brief RX4111リアルタイムクロックモジュール
 */

#ifndef RX4111_H__
#define RX4111_H__

#include <stdint.h>

/**
 * @brief 日時を表す構造体
 */
typedef struct 
{
	uint8_t Year;	/**< 西暦の下二桁 */
	uint8_t Month;	/**< 月 */
	uint8_t Day;	/**< 日 */
	uint8_t Hour;	/**< 時 */
	uint8_t Minute;	/**< 分 */
	uint8_t Sec; 	/**< 秒 */
} RX4111_DATE_TIME;

/**
 * @brief 必要に応じてRX4111の初期化を行う。
 * 
 * @note 通信を行うため、SoftSpiが動作するよう初期化を完了させておく必要がある。
 */
void RX4111Init(void);

/**
 * @brief 日時の設定を行う。
 * 
 * @param dateTime 設定する日時
 */
void RX4111SetTime(const RX4111_DATE_TIME* const dateTime);

/**
 * @brief 日時の取得を行う。
 * 
 * @param dateTime 取得した日時の格納先
 */
void RX4111GetTime(RX4111_DATE_TIME* const dateTime);

#endif // RX4111_H__
