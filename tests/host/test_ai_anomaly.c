/** =================================================================*
 * @file   test_ai_anomaly.c
 * @brief  再構成誤差による「いつもと違う」判定の単体テスト
 * @author Plant Doctor team
 * @date   2026-10
 * ================================================================= */
#include "assert_helper.h"
#include "AiAnomaly.h"

static const AI_ANOMALY_CONFIG CFG = {
    10U,    /* baselineShift: 1/1024 */
    600U,   /* minBaselineSamples */
    15U,    /* minDeviationPpm */
    8U,     /* deviationScale */
    50U,    /* onScore */
    30U,    /* offScore */
    60U,    /* onSamples */
    120U,   /* offSamples */
    60U,    /* relearnEverySamples */
    1800U   /* relearnAfterSamples */
};

/* 健康時の誤差 230〜250ppm を交互に入れて基準を学ばせる */
static void LearnBaseline(AI_ANOMALY_STATE *s, uint32_t n) {
    uint32_t i;
    for (i = 0U; i < n; ++i) {
        AiAnomaly_Update(s, &CFG, (uint16_t)((i % 2U) ? 250U : 230U), true);
    }
}

static void Test_NotReadyBeforeBaseline(void) {
    AI_ANOMALY_STATE s;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 599U);
    TEST_ASSERT_FALSE(AiAnomaly_IsReady(&s, &CFG));
    AiAnomaly_Update(&s, &CFG, 2000U, false);
    TEST_ASSERT_EQUAL_UINT(0U, AiAnomaly_GetScore(&s));
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));
}

static void Test_HealthyStaysLow(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 1000U);
    TEST_ASSERT_TRUE(AiAnomaly_IsReady(&s, &CFG));
    for (i = 0U; i < 300U; ++i) {
        AiAnomaly_Update(&s, &CFG, 255U, true);
        TEST_ASSERT_TRUE(AiAnomaly_GetScore(&s) < 30U);
    }
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));
}

static void Test_DeviationBecomesAnomalyAfterOnSamples(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 1000U);
    for (i = 0U; i < 59U; ++i) {
        AiAnomaly_Update(&s, &CFG, 400U, true);
    }
    TEST_ASSERT_TRUE(AiAnomaly_GetScore(&s) >= 50U);
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));      /* 59件ではまだ判定しない */
    AiAnomaly_Update(&s, &CFG, 400U, true);
    TEST_ASSERT_TRUE(AiAnomaly_IsAnomaly(&s));       /* 60件続いたら判定 */
}

static void Test_BaselineNotLearnedWhileDeviating(void) {
    AI_ANOMALY_STATE s;
    int32_t meanBefore;
    uint32_t i;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 1000U);
    meanBefore = s.meanQ16;
    for (i = 0U; i < 500U; ++i) {
        AiAnomaly_Update(&s, &CFG, 400U, true);      /* 健康扱いでも、外れている間は基準を動かさない */
    }
    TEST_ASSERT_EQUAL_INT(meanBefore, s.meanQ16);
    TEST_ASSERT_TRUE(AiAnomaly_IsAnomaly(&s));
}

static void Test_RecoversAfterOffSamples(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 1000U);
    for (i = 0U; i < 100U; ++i) {
        AiAnomaly_Update(&s, &CFG, 400U, true);
    }
    TEST_ASSERT_TRUE(AiAnomaly_IsAnomaly(&s));
    for (i = 0U; i < 119U; ++i) {
        AiAnomaly_Update(&s, &CFG, 240U, true);
    }
    TEST_ASSERT_TRUE(AiAnomaly_IsAnomaly(&s));       /* 119件ではまだ解除しない */
    AiAnomaly_Update(&s, &CFG, 240U, true);
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));
}

static void Test_InvalidLossIgnored(void) {
    AI_ANOMALY_STATE s;
    uint32_t samples;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 1000U);
    samples = s.baselineSamples;
    AiAnomaly_Update(&s, &CFG, 10000U, true);
    TEST_ASSERT_EQUAL_UINT(samples, s.baselineSamples);
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));
}

static void Test_UnhealthySamplesDoNotBuildBaseline(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    AiAnomaly_Reset(&s);
    for (i = 0U; i < 1000U; ++i) {
        AiAnomaly_Update(&s, &CFG, 240U, false);
    }
    TEST_ASSERT_EQUAL_UINT(0U, s.baselineSamples);
    TEST_ASSERT_FALSE(AiAnomaly_IsReady(&s, &CFG));
}

static void Test_TrainingThrottledWhileDeviating(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    uint32_t trained = 0U;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 1000U);
    AiAnomaly_Update(&s, &CFG, 240U, true);
    TEST_ASSERT_TRUE(AiAnomaly_AllowTraining(&s, &CFG, true));   /* いつも通りなら毎回学ぶ */
    TEST_ASSERT_FALSE(AiAnomaly_AllowTraining(&s, &CFG, false)); /* ルール上だめなら学ばない */
    for (i = 0U; i < 600U; ++i) {
        AiAnomaly_Update(&s, &CFG, 400U, true);
        if (AiAnomaly_AllowTraining(&s, &CFG, true)) {
            ++trained;
        }
    }
    TEST_ASSERT_EQUAL_UINT(10U, trained);                         /* 600件で10回だけ */
}

static void Test_RelearnsAfterLongDeviation(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 1000U);
    for (i = 0U; i < 1799U; ++i) {
        AiAnomaly_Update(&s, &CFG, 400U, true);
        (void)AiAnomaly_AllowTraining(&s, &CFG, true);
    }
    AiAnomaly_Update(&s, &CFG, 400U, true);
    TEST_ASSERT_TRUE(AiAnomaly_AllowTraining(&s, &CFG, true));    /* 30分続いたら毎回学ぶ */
    AiAnomaly_Update(&s, &CFG, 400U, true);
    TEST_ASSERT_TRUE(AiAnomaly_AllowTraining(&s, &CFG, true));
    /* モデルが新しいいつもに慣れて誤差が戻れば、2分後に解除される */
    for (i = 0U; i < 120U; ++i) {
        AiAnomaly_Update(&s, &CFG, 245U, true);
        (void)AiAnomaly_AllowTraining(&s, &CFG, true);
    }
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));
}

static void Test_ClearJudgementKeepsBaseline(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    int32_t meanBefore;
    uint32_t samplesBefore;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 700U);
    for (i = 0U; i < 60U; ++i) {
        AiAnomaly_Update(&s, &CFG, 500U, true);
    }
    TEST_ASSERT_TRUE(AiAnomaly_IsAnomaly(&s));
    meanBefore = s.meanQ16;
    samplesBefore = s.baselineSamples;
    AiAnomaly_ClearJudgement(&s);                      /* 給水後: 判定だけ消す */
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));
    TEST_ASSERT_EQUAL_UINT(0U, AiAnomaly_GetScore(&s));
    TEST_ASSERT_EQUAL_INT(meanBefore, s.meanQ16);      /* 基準は残る */
    TEST_ASSERT_EQUAL_UINT(samplesBefore, s.baselineSamples);
    TEST_ASSERT_TRUE(AiAnomaly_IsReady(&s, &CFG));
    TEST_ASSERT_TRUE(AiAnomaly_AllowTraining(&s, &CFG, true));
}

static void Test_OneDipDoesNotRestartCount(void) {
    AI_ANOMALY_STATE s;
    uint32_t i;
    AiAnomaly_Reset(&s);
    LearnBaseline(&s, 700U);
    for (i = 0U; i < 40U; ++i) {
        AiAnomaly_Update(&s, &CFG, 500U, true);
    }
    AiAnomaly_Update(&s, &CFG, 240U, true);           /* 1回だけいつもの誤差に戻る */
    for (i = 0U; i < 20U; ++i) {
        AiAnomaly_Update(&s, &CFG, 500U, true);
    }
    TEST_ASSERT_FALSE(AiAnomaly_IsAnomaly(&s));       /* 40-1+20=59件 */
    AiAnomaly_Update(&s, &CFG, 500U, true);
    TEST_ASSERT_TRUE(AiAnomaly_IsAnomaly(&s));        /* 60件に届いたら判定（数え直さない） */
}

int main(void) {
    TEST_RUN(Test_NotReadyBeforeBaseline);
    TEST_RUN(Test_ClearJudgementKeepsBaseline);
    TEST_RUN(Test_OneDipDoesNotRestartCount);
    TEST_RUN(Test_HealthyStaysLow);
    TEST_RUN(Test_DeviationBecomesAnomalyAfterOnSamples);
    TEST_RUN(Test_BaselineNotLearnedWhileDeviating);
    TEST_RUN(Test_RecoversAfterOffSamples);
    TEST_RUN(Test_InvalidLossIgnored);
    TEST_RUN(Test_UnhealthySamplesDoNotBuildBaseline);
    TEST_RUN(Test_TrainingThrottledWhileDeviating);
    TEST_RUN(Test_RelearnsAfterLongDeviation);
    TEST_REPORT_AND_EXIT();
}
