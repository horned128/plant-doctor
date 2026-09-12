/** =================================================================*
 * @file   test_time_keeper.c
 * @brief  時刻管理・日時変換単体テスト (P2-5)
 * ================================================================= */
#include "assert_helper.h"
#include "TimeKeeper.h"

static void test_leap_year_2024_round_trip(void) {
    PLANT_DATE_TIME dt = {
        .year = 2024,
        .month = 2,
        .day = 29,
        .hour = 12,
        .minute = 30,
        .second = 45,
        .dayOfWeek = 0
    };

    uint32_t unixSec = TimeKeeper_ToUnixSeconds(&dt);
    TEST_ASSERT_TRUE(unixSec != 0xFFFFFFFFU);

    PLANT_DATE_TIME outDt;
    TimeKeeper_FromUnixSeconds(unixSec, &outDt);

    TEST_ASSERT_EQUAL_UINT(2024, outDt.year);
    TEST_ASSERT_EQUAL_UINT(2, outDt.month);
    TEST_ASSERT_EQUAL_UINT(29, outDt.day);
    TEST_ASSERT_EQUAL_UINT(12, outDt.hour);
    TEST_ASSERT_EQUAL_UINT(30, outDt.minute);
    TEST_ASSERT_EQUAL_UINT(45, outDt.second);
    TEST_ASSERT_EQUAL_UINT(4, outDt.dayOfWeek); /* 2024-02-29 is Thursday (4) */
}

static void test_non_leap_year_2100_march_first(void) {
    /* 2100 is century year not divisible by 400 -> NOT a leap year! */
    PLANT_DATE_TIME invalidFeb29 = {
        .year = 2100,
        .month = 2,
        .day = 29,
        .hour = 0,
        .minute = 0,
        .second = 0
    };
    /* Invalid: 2100-02-29 does not exist */
    TEST_ASSERT_EQUAL_UINT(0xFFFFFFFFU, TimeKeeper_ToUnixSeconds(&invalidFeb29));

    /* Valid: 2100-03-01 */
    PLANT_DATE_TIME mar1 = {
        .year = 2100,
        .month = 3,
        .day = 1,
        .hour = 0,
        .minute = 0,
        .second = 0
    };
    uint32_t unixSec = TimeKeeper_ToUnixSeconds(&mar1);
    TEST_ASSERT_TRUE(unixSec != 0xFFFFFFFFU);

    PLANT_DATE_TIME outDt;
    TimeKeeper_FromUnixSeconds(unixSec, &outDt);
    TEST_ASSERT_EQUAL_UINT(2100, outDt.year);
    TEST_ASSERT_EQUAL_UINT(3, outDt.month);
    TEST_ASSERT_EQUAL_UINT(1, outDt.day);
    TEST_ASSERT_EQUAL_UINT(1, outDt.dayOfWeek); /* 2100-03-01 is Monday (1) */
}

static void test_current_date_round_trip(void) {
    PLANT_DATE_TIME dt = {
        .year = 2026,
        .month = 9,
        .day = 11,
        .hour = 20,
        .minute = 15,
        .second = 30
    };

    uint32_t unixSec = TimeKeeper_ToUnixSeconds(&dt);
    TEST_ASSERT_TRUE(unixSec != 0xFFFFFFFFU);

    PLANT_DATE_TIME outDt;
    TimeKeeper_FromUnixSeconds(unixSec, &outDt);
    TEST_ASSERT_EQUAL_UINT(2026, outDt.year);
    TEST_ASSERT_EQUAL_UINT(9, outDt.month);
    TEST_ASSERT_EQUAL_UINT(11, outDt.day);
    TEST_ASSERT_EQUAL_UINT(20, outDt.hour);
    TEST_ASSERT_EQUAL_UINT(15, outDt.minute);
    TEST_ASSERT_EQUAL_UINT(30, outDt.second);
    TEST_ASSERT_EQUAL_UINT(5, outDt.dayOfWeek); /* 2026-09-11 is Friday (5) */
}

static void test_invalid_and_unsynchronized_handling(void) {
    PLANT_DATE_TIME invalidDt = {
        .year = 1969, /* before epoch */
        .month = 1,
        .day = 1
    };
    TEST_ASSERT_EQUAL_UINT(0xFFFFFFFFU, TimeKeeper_ToUnixSeconds(&invalidDt));

    PLANT_DATE_TIME outDt;
    TimeKeeper_FromUnixSeconds(0xFFFFFFFFU, &outDt);
    TEST_ASSERT_EQUAL_UINT(0U, outDt.year);
    TEST_ASSERT_EQUAL_UINT(0U, outDt.month);

    TimeKeeper_Init(0xFFFFFFFFU, false);
    TEST_ASSERT_FALSE(TimeKeeper_IsSynchronized());
    TEST_ASSERT_EQUAL_UINT(0xFFFFFFFFU, TimeKeeper_GetUnixSeconds());
}

static void test_tick_advances_seconds(void) {
    TimeKeeper_Init(1000U, true);
    TEST_ASSERT_TRUE(TimeKeeper_IsSynchronized());
    TEST_ASSERT_EQUAL_UINT(1000U, TimeKeeper_GetUnixSeconds());

    /* 99 ticks -> still 1000 */
    for (int i = 0; i < 99; ++i) {
        TimeKeeper_Tick10Ms();
    }
    TEST_ASSERT_EQUAL_UINT(1000U, TimeKeeper_GetUnixSeconds());

    /* 100th tick -> 1001 */
    TimeKeeper_Tick10Ms();
    TEST_ASSERT_EQUAL_UINT(1001U, TimeKeeper_GetUnixSeconds());

    /* Another 100 ticks -> 1002 */
    for (int i = 0; i < 100; ++i) {
        TimeKeeper_Tick10Ms();
    }
    TEST_ASSERT_EQUAL_UINT(1002U, TimeKeeper_GetUnixSeconds());

    /* Resynchronize */
    TimeKeeper_Resynchronize(5000U);
    TEST_ASSERT_EQUAL_UINT(5000U, TimeKeeper_GetUnixSeconds());
}

int main(void) {
    test_leap_year_2024_round_trip();
    test_non_leap_year_2100_march_first();
    test_current_date_round_trip();
    test_invalid_and_unsynchronized_handling();
    test_tick_advances_seconds();
    TEST_REPORT_AND_EXIT();
}
