/** =================================================================*
 * @file   TimeKeeper.c
 * @brief  時刻管理・日時変換実装 (P2-5)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "TimeKeeper.h"
#include <stddef.h>

#define SECONDS_PER_MINUTE                  (60UL)
#define SECONDS_PER_HOUR                    (3600UL)
#define SECONDS_PER_DAY                     (86400UL)
#define TICKS_PER_SECOND                    (100U)
#define UNIX_EPOCH_YEAR                     (1970U)

static uint32_t s_currentUnixSeconds;       /**< 現在のUNIX秒 */
static uint8_t s_subTickCount;              /**< 10ms Tickカウンタ (0〜99) */
static bool s_isSynchronized;               /**< RTC同期完了フラグ */

/** =================================================================*
 * @brief  指定年がうるう年かどうかを判定
 * @param  year 西暦年
 * @return うるう年ならtrue
 * ================================================================= */
static bool TimeKeeper_IsLeapYear(uint16_t year) {
    return ((year % 4U == 0U) && ((year % 100U != 0U) || (year % 400U == 0U)));
}

/** =================================================================*
 * @brief  指定年・月の日数を取得
 * @param  year  西暦年
 * @param  month 月 (1〜12)
 * @return 日数 (28〜31)
 * ================================================================= */
static uint8_t TimeKeeper_GetDaysInMonth(uint16_t year, uint8_t month) {
    static const uint8_t s_daysPerMonth[12] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((month < 1U) || (month > 12U)) {
        return 0U;
    }
    if ((month == 2U) && TimeKeeper_IsLeapYear(year)) {
        return 29U;
    }
    return s_daysPerMonth[month - 1U];
}

/** =================================================================*
 * @brief  時刻管理モジュール初期化
 * @param  unixSeconds  初期UNIX秒
 * @param  synchronized 同期状態
 * ================================================================= */
void TimeKeeper_Init(uint32_t unixSeconds, bool synchronized) {
    s_currentUnixSeconds = unixSeconds;
    s_subTickCount = 0U;
    s_isSynchronized = synchronized && (unixSeconds != 0xFFFFFFFFU);
}

/** =================================================================*
 * @brief  10ms周期でTickを加算（100回で1秒繰り上げ）
 * ================================================================= */
void TimeKeeper_Tick10Ms(void) {
    ++s_subTickCount;
    if (s_subTickCount >= TICKS_PER_SECOND) {
        s_subTickCount = 0U;
        if (s_isSynchronized && (s_currentUnixSeconds != 0xFFFFFFFFU)) {
            if (s_currentUnixSeconds < UINT32_MAX) {
                ++s_currentUnixSeconds;
            }
        }
    }
}

/** =================================================================*
 * @brief  現在のUNIX秒を取得
 * @return UNIX秒 (未同期時は0xFFFFFFFF)
 * ================================================================= */
uint32_t TimeKeeper_GetUnixSeconds(void) {
    if (!s_isSynchronized) {
        return 0xFFFFFFFFU;
    }
    return s_currentUnixSeconds;
}

/** =================================================================*
 * @brief  RTC同期済み判定
 * @return 同期済みならtrue
 * ================================================================= */
bool TimeKeeper_IsSynchronized(void) {
    return s_isSynchronized;
}

/** =================================================================*
 * @brief  RTCから取得した時刻で再同期
 * @param  unixSeconds 新規UNIX秒
 * ================================================================= */
void TimeKeeper_Resynchronize(uint32_t unixSeconds) {
    s_currentUnixSeconds = unixSeconds;
    s_subTickCount = 0U;
    s_isSynchronized = (unixSeconds != 0xFFFFFFFFU);
}

/** =================================================================*
 * @brief  日時構造体からUNIX秒を算出
 * @param  dateTime 入力日時
 * @return UNIX秒 (不正時は0xFFFFFFFF)
 * ================================================================= */
uint32_t TimeKeeper_ToUnixSeconds(const PLANT_DATE_TIME *dateTime) {
    uint32_t days = 0U;
    uint16_t y;
    uint8_t m;

    if (dateTime == NULL) {
        return 0xFFFFFFFFU;
    }
    if ((dateTime->year < UNIX_EPOCH_YEAR) ||
        (dateTime->month < 1U) || (dateTime->month > 12U) ||
        (dateTime->day < 1U) || (dateTime->day > 31U) ||
        (dateTime->hour > 23U) || (dateTime->minute > 59U) || (dateTime->second > 59U)) {
        return 0xFFFFFFFFU;
    }
    if (dateTime->day > TimeKeeper_GetDaysInMonth(dateTime->year, dateTime->month)) {
        return 0xFFFFFFFFU;
    }

    /* 1970年からの経過年日数を加算 */
    for (y = UNIX_EPOCH_YEAR; y < dateTime->year; ++y) {
        days += TimeKeeper_IsLeapYear(y) ? 366UL : 365UL;
    }

    /* 当年の経過月日数を加算 */
    for (m = 1U; m < dateTime->month; ++m) {
        days += (uint32_t)TimeKeeper_GetDaysInMonth(dateTime->year, m);
    }

    /* 当月の経過日数を加算 */
    days += (uint32_t)(dateTime->day - 1U);

    return (days * SECONDS_PER_DAY) +
           ((uint32_t)dateTime->hour * SECONDS_PER_HOUR) +
           ((uint32_t)dateTime->minute * SECONDS_PER_MINUTE) +
           (uint32_t)dateTime->second;
}

/** =================================================================*
 * @brief  UNIX秒から日時構造体を算出
 * @param  unixSeconds 入力UNIX秒
 * @param  dateTime    出力日時構造体
 * ================================================================= */
void TimeKeeper_FromUnixSeconds(uint32_t unixSeconds, PLANT_DATE_TIME *dateTime) {
    uint32_t days;
    uint32_t remainingSeconds;
    uint16_t year = UNIX_EPOCH_YEAR;
    uint8_t month = 1U;
    uint16_t daysInCurrentYear;
    uint8_t daysInCurrentMonth;

    if (dateTime == NULL) {
        return;
    }

    if (unixSeconds == 0xFFFFFFFFU) {
        dateTime->year = 0U;
        dateTime->month = 0U;
        dateTime->day = 0U;
        dateTime->hour = 0U;
        dateTime->minute = 0U;
        dateTime->second = 0U;
        dateTime->dayOfWeek = 0U;
        return;
    }

    days = unixSeconds / SECONDS_PER_DAY;
    remainingSeconds = unixSeconds % SECONDS_PER_DAY;

    dateTime->hour = (uint8_t)(remainingSeconds / SECONDS_PER_HOUR);
    remainingSeconds %= SECONDS_PER_HOUR;
    dateTime->minute = (uint8_t)(remainingSeconds / SECONDS_PER_MINUTE);
    dateTime->second = (uint8_t)(remainingSeconds % SECONDS_PER_MINUTE);

    /* 1970-01-01は木曜日 (4) */
    dateTime->dayOfWeek = (uint8_t)((days + 4UL) % 7UL);

    while (1) {
        daysInCurrentYear = TimeKeeper_IsLeapYear(year) ? 366U : 365U;
        if (days < daysInCurrentYear) {
            break;
        }
        days -= daysInCurrentYear;
        ++year;
    }
    dateTime->year = year;

    for (month = 1U; month <= 12U; ++month) {
        daysInCurrentMonth = TimeKeeper_GetDaysInMonth(year, month);
        if (days < daysInCurrentMonth) {
            break;
        }
        days -= daysInCurrentMonth;
    }
    dateTime->month = month;
    dateTime->day = (uint8_t)(days + 1UL);
}
