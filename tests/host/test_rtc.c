/** =================================================================*
 * @file   test_rtc.c
 * @brief  Epson RX4111CE RTC および日時変換単体テスト (P2-5)
 * ================================================================= */
#include <assert.h>
#include <stdio.h>
#include "RtcRx4111.h"

static void test_conversion_roundtrip(void) {
    RTC_RX4111_TIME inTime = {
        .Year = 26U,   /* 2026 */
        .Month = 9U,
        .Day = 12U,
        .Hour = 14U,
        .Minute = 30U,
        .Sec = 45U
    };

    uint32_t unixSec = RtcRx4111_TimeToUnix(&inTime);
    assert(unixSec > 0U);

    RTC_RX4111_TIME outTime;
    RtcRx4111_UnixToTime(unixSec, &outTime);

    assert(outTime.Year == inTime.Year);
    assert(outTime.Month == inTime.Month);
    assert(outTime.Day == inTime.Day);
    assert(outTime.Hour == inTime.Hour);
    assert(outTime.Minute == inTime.Minute);
    assert(outTime.Sec == inTime.Sec);

    printf("PASS: test_conversion_roundtrip (Unix = %lu)\n", (unsigned long)unixSec);
}

static void test_leap_year_february(void) {
    /* 2024年（うるう年）の 2月29日 */
    RTC_RX4111_TIME leapFeb29 = {
        .Year = 24U,
        .Month = 2U,
        .Day = 29U,
        .Hour = 12U,
        .Minute = 0U,
        .Sec = 0U
    };
    uint32_t sec = RtcRx4111_TimeToUnix(&leapFeb29);
    RTC_RX4111_TIME out;
    RtcRx4111_UnixToTime(sec, &out);
    assert(out.Year == 24U);
    assert(out.Month == 2U);
    assert(out.Day == 29U);

    /* 翌日: 3月1日 */
    RtcRx4111_UnixToTime(sec + 86400UL, &out);
    assert(out.Year == 24U);
    assert(out.Month == 3U);
    assert(out.Day == 1U);

    printf("PASS: test_leap_year_february\n");
}

static void test_rtc_set_get_simulated(void) {
    RTC_RX4111_TIME setTime = {
        .Year = 26U,
        .Month = 10U,
        .Day = 1U,
        .Hour = 8U,
        .Minute = 15U,
        .Sec = 0U
    };
    assert(RtcRx4111_SetTime(&setTime));

    RTC_RX4111_TIME getTime;
    assert(RtcRx4111_GetTime(&getTime));

    assert(getTime.Year == 26U);
    assert(getTime.Month == 10U);
    assert(getTime.Day == 1U);
    assert(getTime.Hour == 8U);
    assert(getTime.Minute == 15U);
    assert(getTime.Sec == 0U);

    printf("PASS: test_rtc_set_get_simulated\n");
}

int main(void) {
    printf("=== test_rtc ===\n");
    test_conversion_roundtrip();
    test_leap_year_february();
    test_rtc_set_get_simulated();
    printf("All test_rtc passed.\n");
    return 0;
}
