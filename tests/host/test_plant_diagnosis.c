/** =================================================================*
 * @file   test_plant_diagnosis.c
 * @brief  植物診断・原因候補推定の単体テスト (P4-2)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"
#include "PlantDiagnosis.h"

/* テスト用設定定数（マクロ値ではなくテスト固有定数を渡す） */
static const PLANT_DIAGNOSIS_CONFIG TEST_CONFIG = {
    300,    /* dryThresholdPermille: 300‰ */
    200,    /* heatStressTempDelta: +2.00℃ (200 centi-C) */
    50,     /* heatStressRatePerHour: +0.50℃/h */
    1000,   /* lowLightAccumulatedThreshold: 1000 */
    400,    /* rootUptakeSoilMinPermille: 400‰ */
    150,    /* rootUptakeTempDelta: +1.50℃ */
    -10,    /* soilDryRatePerHour: -10‰/h */
    10      /* soilWetRatePerHour: +10‰/h */
};

static void SetBaselineNormalInput(PLANT_DIAGNOSIS_INPUT *input) {
    input->features.soilMoisturePermille = 500;
    input->features.soilMoistureMovingAverage = 500;
    input->features.soilMoistureRatePerHour = 0;
    input->features.leafAirTemperatureDelta = 50;   /* +0.50℃ */
    input->features.leafTemperatureRatePerHour = 0;
    input->features.illuminanceRaw = 500;
    input->features.illuminanceAccumulated = 5000;
    input->features.relativeHumidityCentiPercent = 6000;
    input->features.elapsedSinceWateringSeconds = 3600;
    input->features.validMask = 0xFF;

    input->soilHealth = SENSOR_HEALTH_OK;
    input->leafHealth = SENSOR_HEALTH_OK;
    input->airHumHealth = SENSOR_HEALTH_OK;
    input->luxHealth = SENSOR_HEALTH_OK;

    input->isWateringActive = false;
    input->isWateringFailed = false;
    input->isSoilDegraded = false;
    input->wateringDidNotCoolLeaf = false;
}

/** =================================================================*
 * @brief  正常（Healthy）および初期化の検証
 * ================================================================= */
static void Test_HealthyAndReset(void) {
    PLANT_DIAGNOSIS_STATE state;
    PLANT_DIAGNOSIS_INPUT input;

    PlantDiagnosis_Reset(&state);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_HEALTHY, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_UNKNOWN, PlantDiagnosis_GetSoilTrend(&state));
    TEST_ASSERT_EQUAL_INT(DIAGNOSIS_FAILED_SENSOR_NONE, PlantDiagnosis_GetFailedSensor(&state));

    SetBaselineNormalInput(&input);
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);

    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_HEALTHY, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_STABLE, PlantDiagnosis_GetSoilTrend(&state));
    TEST_ASSERT_EQUAL_INT(DIAGNOSIS_FAILED_SENSOR_NONE, PlantDiagnosis_GetFailedSensor(&state));
}

/** =================================================================*
 * @brief  各原因候補の単独判定検証
 * ================================================================= */
static void Test_IndividualCandidates(void) {
    PLANT_DIAGNOSIS_STATE state;
    PLANT_DIAGNOSIS_INPUT input;

    /* 1. 土壌の乾燥 (Dry Stress) */
    SetBaselineNormalInput(&input);
    input.features.soilMoisturePermille = 250;   /* < 300 */
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_DRY_STRESS, PlantDiagnosis_GetStatus(&state));

    /* 2. 熱ストレス (DEMO2: 葉温-気温差+2.0℃ かつ 上昇速度大) */
    SetBaselineNormalInput(&input);
    input.features.leafAirTemperatureDelta = 200;    /* == 200 */
    input.features.leafTemperatureRatePerHour = 60;  /* > 50 */
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_HEAT_STRESS, PlantDiagnosis_GetStatus(&state));

    /* 3. 日照不足 (Low Light: 積算照度不足) */
    SetBaselineNormalInput(&input);
    input.features.illuminanceAccumulated = 800;     /* < 1000 */
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_LOW_LIGHT, PlantDiagnosis_GetStatus(&state));

    /* 4. 根の吸水不良 (Root Uptake: 土壌水分十分 + 葉温差大 + 給水後葉温低下なし) */
    SetBaselineNormalInput(&input);
    input.features.soilMoisturePermille = 600;       /* >= 400 */
    input.features.leafAirTemperatureDelta = 160;    /* >= 150 */
    input.wateringDidNotCoolLeaf = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_ROOT_UPTAKE, PlantDiagnosis_GetStatus(&state));

    /* 5. 水やり動作中 (Watering) */
    SetBaselineNormalInput(&input);
    input.isWateringActive = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_WATERING, PlantDiagnosis_GetStatus(&state));

    /* 6. 水やり失敗 (Watering Failed) */
    SetBaselineNormalInput(&input);
    input.isWateringFailed = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_WATERING_FAILED, PlantDiagnosis_GetStatus(&state));

    /* 7. 土壌劣化 (Soil Degradation) */
    SetBaselineNormalInput(&input);
    input.isSoilDegraded = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_SOIL_DEGRADATION, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_DEGRADATION, PlantDiagnosis_GetSoilTrend(&state));
}

/** =================================================================*
 * @brief  優先順位（Priority Hierarchy）の検証
 * SENSOR_ERROR > WATERING_FAILED > WATERING > DRY_STRESS >
 * SOIL_DEGRADATION > HEAT_STRESS > ROOT_UPTAKE > LOW_LIGHT > HEALTHY
 * ================================================================= */
static void Test_PriorityHierarchy(void) {
    PLANT_DIAGNOSIS_STATE state;
    PLANT_DIAGNOSIS_INPUT input;

    /* SENSOR_ERROR > WATERING_FAILED */
    SetBaselineNormalInput(&input);
    input.soilHealth = SENSOR_HEALTH_OUT_OF_RANGE;
    input.isWateringFailed = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_SENSOR_ERROR, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(DIAGNOSIS_FAILED_SENSOR_SOIL, PlantDiagnosis_GetFailedSensor(&state));

    /* WATERING_FAILED > WATERING */
    SetBaselineNormalInput(&input);
    input.isWateringFailed = true;
    input.isWateringActive = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_WATERING_FAILED, PlantDiagnosis_GetStatus(&state));

    /* WATERING > DRY_STRESS */
    SetBaselineNormalInput(&input);
    input.isWateringActive = true;
    input.features.soilMoisturePermille = 200;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_WATERING, PlantDiagnosis_GetStatus(&state));

    /* DRY_STRESS > SOIL_DEGRADATION */
    SetBaselineNormalInput(&input);
    input.features.soilMoisturePermille = 200;
    input.isSoilDegraded = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_DRY_STRESS, PlantDiagnosis_GetStatus(&state));

    /* SOIL_DEGRADATION > HEAT_STRESS */
    SetBaselineNormalInput(&input);
    input.isSoilDegraded = true;
    input.features.leafAirTemperatureDelta = 250;
    input.features.leafTemperatureRatePerHour = 100;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_SOIL_DEGRADATION, PlantDiagnosis_GetStatus(&state));

    /* HEAT_STRESS > ROOT_UPTAKE */
    SetBaselineNormalInput(&input);
    input.features.leafAirTemperatureDelta = 250;
    input.features.leafTemperatureRatePerHour = 100;
    input.wateringDidNotCoolLeaf = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_HEAT_STRESS, PlantDiagnosis_GetStatus(&state));

    /* ROOT_UPTAKE > LOW_LIGHT */
    SetBaselineNormalInput(&input);
    input.features.leafAirTemperatureDelta = 160;
    input.wateringDidNotCoolLeaf = true;
    input.features.illuminanceAccumulated = 500;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_ROOT_UPTAKE, PlantDiagnosis_GetStatus(&state));

    /* LOW_LIGHT > HEALTHY */
    SetBaselineNormalInput(&input);
    input.features.illuminanceAccumulated = 500;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_LOW_LIGHT, PlantDiagnosis_GetStatus(&state));
}

/** =================================================================*
 * @brief  センサー異常グループの特定と優先順位の検証 (DEMO5)
 * ================================================================= */
static void Test_SensorErrorIdentification(void) {
    PLANT_DIAGNOSIS_STATE state;
    PLANT_DIAGNOSIS_INPUT input;

    /* 土壌センサー異常 (DEMO5: 土壌水分センサを抜いた状態) */
    SetBaselineNormalInput(&input);
    input.soilHealth = SENSOR_HEALTH_OUT_OF_RANGE;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_SENSOR_ERROR, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(DIAGNOSIS_FAILED_SENSOR_SOIL, PlantDiagnosis_GetFailedSensor(&state));

    /* 葉温センサー異常 */
    SetBaselineNormalInput(&input);
    input.leafHealth = SENSOR_HEALTH_NO_COMMUNICATION;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_SENSOR_ERROR, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(DIAGNOSIS_FAILED_SENSOR_LEAF, PlantDiagnosis_GetFailedSensor(&state));

    /* 気温・湿度センサー異常 */
    SetBaselineNormalInput(&input);
    input.airHumHealth = SENSOR_HEALTH_INCONSISTENT;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_SENSOR_ERROR, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(DIAGNOSIS_FAILED_SENSOR_AIR_HUM, PlantDiagnosis_GetFailedSensor(&state));

    /* 照度センサー異常 (DEMO5: 照度センサを覆う) */
    SetBaselineNormalInput(&input);
    input.luxHealth = SENSOR_HEALTH_INCONSISTENT;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(PLANT_STATUS_SENSOR_ERROR, PlantDiagnosis_GetStatus(&state));
    TEST_ASSERT_EQUAL_INT(DIAGNOSIS_FAILED_SENSOR_LUX, PlantDiagnosis_GetFailedSensor(&state));
}

/** =================================================================*
 * @brief  土壌傾向（Soil Trend）の判定検証
 * ================================================================= */
static void Test_SoilTrend(void) {
    PLANT_DIAGNOSIS_STATE state;
    PLANT_DIAGNOSIS_INPUT input;

    /* 1. 履歴不足 (UNKNOWN) */
    SetBaselineNormalInput(&input);
    input.features.validMask = (uint8_t)(input.features.validMask & 0xFBU);
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_UNKNOWN, PlantDiagnosis_GetSoilTrend(&state));

    /* 2. 乾燥傾向 (DRY: rate <= -10) */
    SetBaselineNormalInput(&input);
    input.features.soilMoistureRatePerHour = -15;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_DRY, PlantDiagnosis_GetSoilTrend(&state));

    /* 3. 湿潤傾向 (WET: rate >= +10) */
    SetBaselineNormalInput(&input);
    input.features.soilMoistureRatePerHour = 20;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_WET, PlantDiagnosis_GetSoilTrend(&state));

    /* 4. 安定 (STABLE: -10 < rate < +10) */
    SetBaselineNormalInput(&input);
    input.features.soilMoistureRatePerHour = -5;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_STABLE, PlantDiagnosis_GetSoilTrend(&state));

    /* 5. 劣化 (DEGRADATION) */
    SetBaselineNormalInput(&input);
    input.isSoilDegraded = true;
    PlantDiagnosis_Update(&state, &input, &TEST_CONFIG);
    TEST_ASSERT_EQUAL_INT(SOIL_TREND_DEGRADATION, PlantDiagnosis_GetSoilTrend(&state));
}

int main(void) {
    TEST_RUN(Test_HealthyAndReset);
    TEST_RUN(Test_IndividualCandidates);
    TEST_RUN(Test_PriorityHierarchy);
    TEST_RUN(Test_SensorErrorIdentification);
    TEST_RUN(Test_SoilTrend);
    TEST_REPORT_AND_EXIT();
}

