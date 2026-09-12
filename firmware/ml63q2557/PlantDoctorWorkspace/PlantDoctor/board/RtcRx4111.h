/** =================================================================*
 * @file   RtcRx4111.h
 * @brief  Epson RX4111CE リアルタイムクロックドライバ
 * ================================================================= */
#ifndef RTC_RX4111_H
#define RTC_RX4111_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief RTC 日時構造体
 */
typedef struct {
    uint8_t Year;                      /**< 西暦下2桁 (0..99, 2000..2099) */
    uint8_t Month;                     /**< 月 (1..12) */
    uint8_t Day;                       /**< 日 (1..31) */
    uint8_t Hour;                      /**< 時 (0..23) */
    uint8_t Minute;                    /**< 分 (0..59) */
    uint8_t Sec;                       /**< 秒 (0..59) */
} RTC_RX4111_TIME;

/**
 * @brief RX4111 の初期化
 */
void RtcRx4111_Init(void);

/**
 * @brief 日時の設定
 */
bool RtcRx4111_SetTime(const RTC_RX4111_TIME *time);

/**
 * @brief 日時の取得
 */
bool RtcRx4111_GetTime(RTC_RX4111_TIME *time);

/**
 * @brief RTC 日時構造体から Unix 秒への変換
 */
uint32_t RtcRx4111_TimeToUnix(const RTC_RX4111_TIME *time);

/**
 * @brief Unix 秒から RTC 日時構造体への変換
 */
void RtcRx4111_UnixToTime(uint32_t unixSec, RTC_RX4111_TIME *time);

#endif /* RTC_RX4111_H */
