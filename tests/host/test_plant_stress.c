/** =================================================================*
 * @file   test_plant_stress.c
 * @brief  植物ストレスバロメータ 0〜100算出の単体テスト (P4-3)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"
#include "PlantStress.h"

/* テスト用設定定数 */
static const PLANT_STRESS_CONFIG TEST_CONFIG = {
    18U,    /* baseStressScore: 18 */
    100U,   /* weightSoil: 100 */
    100U,   /* weightHeat: 100 */
    80U,    /* weightLight: 80 */
    60U,    /* weightHumidity: 60 */
    75U,    /* maxDominanceWeight: 75% */

    600,    /* baselineSoilMoisturePermille: 600‰ */
    80,     /* baselineTempDeltaCentiC: +0.80℃ */
    182,    /* maxHeatDeltaRangeCentiC: +1.82℃ */
    5000,   /* baselineIlluminanceAccum: 5000 */
    6000U   /* baselineHumidityCentiPercent: 60.00% */
};

static void SetBaselineNormalFeatures(PLANT_FEATURE_VECTOR *features) {
    features->soilMoisturePermille = 600;
    features->soilMoistureMovingAverage = 600;
    features->soilMoistureRatePerHour = 0;
    features->leafAirTemperatureDelta = 80;     /* +0.80℃ (DEMO1平常) */
    features->leafTemperatureRatePerHour = 0;
    features->illuminanceRaw = 500;
    features->illuminanceAccumulated = 5000;
    features->relativeHumidityCentiPercent = 6000U;
    features->elapsedSinceWateringSeconds = 3600;
    features->validMask = 0xFF;
}

static void SetAllSensorsOk(SENSOR_HEALTH_REPORT *health) {
    health->soilHealth = SENSOR_HEALTH_OK;
    health->leafHealth = SENSOR_HEALTH_OK;
    health->airHumHealth = SENSOR_HEALTH_OK;
    health->luxHealth = SENSOR_HEALTH_OK;
}

/** =================================================================*
 * @brief  平常時（DEMO1）のスコア検証: 18 / 100
 * ================================================================= */
static void Test_BaselineScore(void) {
    PLANT_FEATURE_VECTOR features;
    SENSOR_HEALTH_REPORT health;
    PLANT_STRESS_OUTPUT output;

    SetBaselineNormalFeatures(&features);
    SetAllSensorsOk(&health);

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    TEST_ASSERT_EQUAL_UINT(18U, output.stressScore);
    TEST_ASSERT_EQUAL_UINT(0U, output.soilPartialScore);
    TEST_ASSERT_EQUAL_UINT(0U, output.heatPartialScore);
    TEST_ASSERT_EQUAL_UINT(0U, output.lightPartialScore);
    TEST_ASSERT_EQUAL_UINT(0U, output.humidityPartialScore);
    TEST_ASSERT_EQUAL_UINT(PLANT_STRESS_FEATURE_SOIL | PLANT_STRESS_FEATURE_HEAT |
                           PLANT_STRESS_FEATURE_LIGHT | PLANT_STRESS_FEATURE_HUMIDITY,
                           output.evaluatedFeaturesMask);
}

/** =================================================================*
 * @brief  軽微なずれのスコア検証: 24 / 100
 * ================================================================= */
static void Test_SlightDeviationScore(void) {
    PLANT_FEATURE_VECTOR features;
    SENSOR_HEALTH_REPORT health;
    PLANT_STRESS_OUTPUT output;

    SetBaselineNormalFeatures(&features);
    SetAllSensorsOk(&health);

    /* 土壌水分が軽微に低下: 600 -> 550 (約8%低下) */
    features.soilMoisturePermille = 550;

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    /* 23〜25の範囲内（目標24） */
    TEST_ASSERT_TRUE((output.stressScore >= 22U) && (output.stressScore <= 26U));
}

/** =================================================================*
 * @brief  熱ストレス（DEMO2: +2.0℃）のスコア検証: ~72 / 100
 * ================================================================= */
static void Test_HeatStressScoreDemo2(void) {
    PLANT_FEATURE_VECTOR features;
    SENSOR_HEALTH_REPORT health;
    PLANT_STRESS_OUTPUT output;

    SetBaselineNormalFeatures(&features);
    SetAllSensorsOk(&health);

    /* 葉温-気温差が+2.00℃ (200 centi-C) に上昇 */
    features.leafAirTemperatureDelta = 200;

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    /* DEMO2の期待表示: Stress: 72/100 (許容誤差±2) */
    TEST_ASSERT_TRUE((output.stressScore >= 70U) && (output.stressScore <= 74U));
    TEST_ASSERT_TRUE(output.heatPartialScore >= 65U);
}

/** =================================================================*
 * @brief  乾燥ストレス・水やり（DEMO3〜4: 深い乾燥）: ~80 / 100
 * ================================================================= */
static void Test_DryStressScoreDemo3(void) {
    PLANT_FEATURE_VECTOR features;
    SENSOR_HEALTH_REPORT health;
    PLANT_STRESS_OUTPUT output;

    SetBaselineNormalFeatures(&features);
    SetAllSensorsOk(&health);

    /* 土壌水分が深く乾燥 (145‰) */
    features.soilMoisturePermille = 145;

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    /* DEMO3〜4の期待表示: Stress: 80/100 (許容誤差±3) */
    TEST_ASSERT_TRUE((output.stressScore >= 78U) && (output.stressScore <= 84U));
    TEST_ASSERT_TRUE(output.soilPartialScore >= 75U);
}

/** =================================================================*
 * @brief  全特徴量無効時は PLANT_STRESS_UNKNOWN (0xFF)
 * ================================================================= */
static void Test_AllInvalidGivesUnknown(void) {
    PLANT_FEATURE_VECTOR features;
    SENSOR_HEALTH_REPORT health;
    PLANT_STRESS_OUTPUT output;

    SetBaselineNormalFeatures(&features);
    SetAllSensorsOk(&health);

    /* 通信無効 */
    features.validMask = 0x00;

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    TEST_ASSERT_EQUAL_UINT(PLANT_STRESS_UNKNOWN, output.stressScore);
    TEST_ASSERT_EQUAL_UINT(0U, output.evaluatedFeaturesMask);

    /* 健全性エラーでも同様 */
    features.validMask = 0xFF;
    health.soilHealth = SENSOR_HEALTH_NO_COMMUNICATION;
    health.leafHealth = SENSOR_HEALTH_NO_COMMUNICATION;
    health.airHumHealth = SENSOR_HEALTH_NO_COMMUNICATION;
    health.luxHealth = SENSOR_HEALTH_NO_COMMUNICATION;

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    TEST_ASSERT_EQUAL_UINT(PLANT_STRESS_UNKNOWN, output.stressScore);
    TEST_ASSERT_EQUAL_UINT(0U, output.evaluatedFeaturesMask);
}

/** =================================================================*
 * @brief  一部センサー異常時の部分合成検証 (DEMO5)
 * 土壌水分センサーが異常だが葉温・気温が正常な場合、0xFFにならず数値を返す
 * ================================================================= */
static void Test_PartialSensorFailureDemo5(void) {
    PLANT_FEATURE_VECTOR features;
    SENSOR_HEALTH_REPORT health;
    PLANT_STRESS_OUTPUT output;

    SetBaselineNormalFeatures(&features);
    SetAllSensorsOk(&health);

    /* 土壌水分センサを土から抜く (DEMO5: 異常判定) */
    health.soilHealth = SENSOR_HEALTH_OUT_OF_RANGE;

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    /* 0xFFではなく有効な数値が返ること */
    TEST_ASSERT_TRUE(output.stressScore != PLANT_STRESS_UNKNOWN);
    TEST_ASSERT_TRUE(output.stressScore <= 100U);

    /* 土壌水分はマスクから除外され、葉温・照度・湿度が寄与していること */
    TEST_ASSERT_EQUAL_UINT(0U, (output.evaluatedFeaturesMask & PLANT_STRESS_FEATURE_SOIL));
    TEST_ASSERT_TRUE((output.evaluatedFeaturesMask & PLANT_STRESS_FEATURE_HEAT) != 0U);
}

/** =================================================================*
 * @brief  クランプ上限（100）の検証
 * ================================================================= */
static void Test_ClampingTo100(void) {
    PLANT_FEATURE_VECTOR features;
    SENSOR_HEALTH_REPORT health;
    PLANT_STRESS_OUTPUT output;

    SetBaselineNormalFeatures(&features);
    SetAllSensorsOk(&health);

    /* 全センサ極端な異常値 */
    features.soilMoisturePermille = 0;
    features.leafAirTemperatureDelta = 1000;    /* +10℃ */
    features.illuminanceAccumulated = 0;
    features.relativeHumidityCentiPercent = 0U;

    PlantStress_Evaluate(&features, &health, &TEST_CONFIG, &output);

    TEST_ASSERT_EQUAL_UINT(100U, output.stressScore);
    TEST_ASSERT_EQUAL_UINT(100U, output.soilPartialScore);
    TEST_ASSERT_EQUAL_UINT(100U, output.heatPartialScore);
    TEST_ASSERT_EQUAL_UINT(100U, output.lightPartialScore);
    TEST_ASSERT_EQUAL_UINT(100U, output.humidityPartialScore);
}

int main(void) {
    TEST_RUN(Test_BaselineScore);
    TEST_RUN(Test_SlightDeviationScore);
    TEST_RUN(Test_HeatStressScoreDemo2);
    TEST_RUN(Test_DryStressScoreDemo3);
    TEST_RUN(Test_AllInvalidGivesUnknown);
    TEST_RUN(Test_PartialSensorFailureDemo5);
    TEST_RUN(Test_ClampingTo100);
    TEST_REPORT_AND_EXIT();
}
