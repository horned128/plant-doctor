/** =================================================================*
 * @file   assert_helper.h
 * @brief  ホスト単体テスト用アサーションマクロ
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef ASSERT_HELPER_H
#define ASSERT_HELPER_H
#include <stdbool.h>                        /* 真偽値型定義 */
#include <stdint.h>                         /* 固定幅整数型定義 */
#include <stdio.h>                          /* 標準入出力 */
#include <stdlib.h>                         /* ユーティリティ関数 */
#include <string.h>                         /* 文字列操作関数 */

/* テスト失敗回数カウンタ */
static int s_test_failures = 0;                             /**< テスト失敗総数 */

#define TEST_ASSERT_TRUE(cond)                                                 \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL: %s:%d: TEST_ASSERT_TRUE(%s)\n",             \
                    __FILE__, __LINE__, #cond);                                \
            s_test_failures++;                                                 \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_FALSE(cond)                                                \
    do {                                                                       \
        if (cond) {                                                            \
            fprintf(stderr, "FAIL: %s:%d: TEST_ASSERT_FALSE(%s)\n",            \
                    __FILE__, __LINE__, #cond);                                \
            s_test_failures++;                                                 \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQUAL_INT(exp, act)                                        \
    do {                                                                       \
        int64_t expected_val_ = (int64_t)(exp);                                \
        int64_t actual_val_ = (int64_t)(act);                                  \
        if (expected_val_ != actual_val_) {                                    \
            fprintf(stderr,                                                    \
                    "FAIL: %s:%d: expected %lld, got %lld (%s == %s)\n",       \
                    __FILE__, __LINE__, (long long)expected_val_,              \
                    (long long)actual_val_, #exp, #act);                       \
            s_test_failures++;                                                 \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQUAL_UINT(exp, act)                                       \
    do {                                                                       \
        uint64_t expected_val_ = (uint64_t)(exp);                              \
        uint64_t actual_val_ = (uint64_t)(act);                                \
        if (expected_val_ != actual_val_) {                                    \
            fprintf(stderr,                                                    \
                    "FAIL: %s:%d: expected %llu, got %llu (%s == %s)\n",       \
                    __FILE__, __LINE__,                                        \
                    (unsigned long long)expected_val_,                         \
                    (unsigned long long)actual_val_, #exp, #act);              \
            s_test_failures++;                                                 \
        }                                                                      \
    } while (0)

#define TEST_ASSERT_EQUAL_STRING(exp, act)                                     \
    do {                                                                       \
        const char *expected_str_ = (exp);                                     \
        const char *actual_str_ = (act);                                       \
        if (strcmp(expected_str_, actual_str_) != 0) {                         \
            fprintf(stderr,                                                    \
                    "FAIL: %s:%d: expected \"%s\", got \"%s\" (%s == %s)\n",   \
                    __FILE__, __LINE__, expected_str_, actual_str_,            \
                    #exp, #act);                                               \
            s_test_failures++;                                                 \
        }                                                                      \
    } while (0)

#define TEST_RUN(func)                                                         \
    do {                                                                       \
        printf("RUN: %s\n", #func);                                            \
        func();                                                                \
    } while (0)

#define TEST_REPORT_AND_EXIT()                                                 \
    do {                                                                       \
        if (s_test_failures > 0) {                                             \
            fprintf(stderr, "TEST FAILED with %d failure(s)\n",                \
                    s_test_failures);                                          \
            return 1;                                                          \
        }                                                                      \
        printf("ALL TESTS PASSED\n");                                          \
        return 0;                                                              \
    } while (0)

#endif /* ASSERT_HELPER_H */
