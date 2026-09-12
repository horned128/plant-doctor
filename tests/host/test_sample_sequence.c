/** =================================================================*
 * @file   test_sample_sequence.c
 * @brief  センササンプル連番判定単体テスト
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"                  /* ホストテスト用アサーションマクロ */

/** =================================================================*
 * @brief  連番比較による新規サンプル判定ロジック
 * @param[in] currentSequence 最新の連番
 * @param[in] lastSeenSequence 前回処理した連番
 * @return 新規サンプルの場合true
 * ================================================================= */
static bool CheckIsNewSample(uint16_t currentSequence, uint16_t lastSeenSequence) {
    return currentSequence != lastSeenSequence;
}

/** =================================================================*
 * @brief  通常インクリメント時の判定検証
 * ================================================================= */
static void Test_NormalSequenceProgression(void) {
    uint16_t lastSeen = 0U;
    uint16_t current = 1U;

    TEST_ASSERT_TRUE(CheckIsNewSample(current, lastSeen));

    /* 前回と同じ連番の場合は新規でない */
    lastSeen = current;
    TEST_ASSERT_FALSE(CheckIsNewSample(current, lastSeen));

    /* 次の連番に進むと新規 */
    current = 2U;
    TEST_ASSERT_TRUE(CheckIsNewSample(current, lastSeen));
}

/** =================================================================*
 * @brief  ラップアラウンド（65535 -> 0）時の判定検証
 * ================================================================= */
static void Test_WraparoundProgression(void) {
    uint16_t lastSeen = 65535U;
    uint16_t current = 0U;

    /* 大小比較 (current > lastSeen) だと誤判定するが、!= 比較なら正しく新規と判定される */
    TEST_ASSERT_TRUE(CheckIsNewSample(current, lastSeen));

    /* 0で処理完了後、同じ0なら新規ではない */
    lastSeen = current;
    TEST_ASSERT_FALSE(CheckIsNewSample(current, lastSeen));

    /* 0 -> 1 */
    current = 1U;
    TEST_ASSERT_TRUE(CheckIsNewSample(current, lastSeen));
}

/** =================================================================*
 * @brief  テストメインエントリ
 * @return 成功時0、失敗時1
 * ================================================================= */
int main(void) {
    TEST_RUN(Test_NormalSequenceProgression);
    TEST_RUN(Test_WraparoundProgression);

    TEST_REPORT_AND_EXIT();
}
