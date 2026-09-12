/** =================================================================*
 * @file   test_smoke.c
 * @brief  ホスト単体テスト基盤スモークテスト
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"                  /* ホストテスト用アサーションマクロ */

/** =================================================================*
 * @brief  真偽値アサーションの検証
 * ================================================================= */
static void Test_AssertBooleans(void) {
    TEST_ASSERT_TRUE(true);
    TEST_ASSERT_FALSE(false);
    TEST_ASSERT_TRUE(1 == 1);
    TEST_ASSERT_FALSE(1 == 2);
}

/** =================================================================*
 * @brief  整数アサーションの検証
 * ================================================================= */
static void Test_AssertIntegers(void) {
    int32_t negative_val = -123;
    uint32_t unsigned_val = 456U;

    TEST_ASSERT_EQUAL_INT(-123, negative_val);
    TEST_ASSERT_EQUAL_UINT(456U, unsigned_val);
    TEST_ASSERT_EQUAL_INT(0, 0);
}

/** =================================================================*
 * @brief  文字列アサーションの検証
 * ================================================================= */
static void Test_AssertStrings(void) {
    const char *sample = "PlantDoctor";

    TEST_ASSERT_EQUAL_STRING("PlantDoctor", sample);
}

/** =================================================================*
 * @brief  スモークテストメインエントリ
 * @return 成功時0、失敗時1
 * ================================================================= */
int main(void) {
    TEST_RUN(Test_AssertBooleans);
    TEST_RUN(Test_AssertIntegers);
    TEST_RUN(Test_AssertStrings);

    TEST_REPORT_AND_EXIT();
}
