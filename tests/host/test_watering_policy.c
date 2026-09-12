/** =================================================================*
 * @file   test_watering_policy.c
 * @brief  自律水やり判定ポリシー単体テスト (P5-5)
 * ================================================================= */
#include "assert_helper.h"
#include "WateringPolicy.h"

static WATERING_POLICY_CONFIG s_defaultConfig = {
    .dryThresholdPermille = 300,
    .minIntervalSeconds = 1800U,
    .minIntervalTicks = 180000UL,
    .maxDailyWateringCount = 6U,
    .autoWateringEnabled = true
};

static void test_all_conditions_satisfied_requests_watering(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 10000U,
        .currentTick = 1000000UL
    };

    WateringPolicy_Reset(&state);
    state.lastWateringSeconds = 5000U; /* 5000s > 1800s */
    state.lastWateringTick = 500000UL;
    state.dailyWateringCount = 2U;
    state.lastDayIndex = 0U; /* same day */

    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_REQUEST, decision);
}

static void test_condition1_not_dry_holds(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_HEALTHY, /* not dry stress */
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 10000U,
        .currentTick = 1000000UL
    };

    WateringPolicy_Reset(&state);
    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_HOLD, decision);

    /* Status is DRY_STRESS but soil moisture is above threshold */
    input.plantStatus = PLANT_STATUS_DRY_STRESS;
    input.soilMoisturePermille = 350; /* >= 300 */
    decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_HOLD, decision);
}

static void test_condition2_sensor_error_blocks(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OUT_OF_RANGE, /* invalid sensor! */
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 10000U,
        .currentTick = 1000000UL
    };

    WateringPolicy_Reset(&state);
    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);
}

static void test_condition3_tank_empty_blocks(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = false, /* empty tank! */
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 10000U,
        .currentTick = 1000000UL
    };

    WateringPolicy_Reset(&state);
    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);
}

static void test_condition4_interval_not_elapsed_blocks(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 2000U,
        .currentTick = 200000UL
    };

    WateringPolicy_Reset(&state);
    state.lastWateringSeconds = 1000U; /* only 1000s elapsed < 1800s */
    state.lastWateringTick = 100000UL;

    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);
}

static void test_condition5_last_watering_failed_blocks(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_FAILED, /* failed! */
        .nowSeconds = 10000U,
        .currentTick = 1000000UL
    };

    WateringPolicy_Reset(&state);
    state.lastWateringSeconds = 5000U;
    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);
}

static void test_condition6_daily_limit_reached_blocks(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 50000U,
        .currentTick = 5000000UL
    };

    WateringPolicy_Reset(&state);
    state.lastWateringSeconds = 40000U;
    state.dailyWateringCount = 6U; /* maximum 6 */
    state.lastDayIndex = 0U; /* same day */

    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);

    /* Next day: daily count resets automatically */
    input.nowSeconds = 50000U + 86400U;
    decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_REQUEST, decision);
    TEST_ASSERT_EQUAL_UINT(0U, state.dailyWateringCount);
}

static void test_condition7_not_monitoring_blocks(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = false, /* not monitoring (e.g. self-test or error) */
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 10000U,
        .currentTick = 1000000UL
    };

    WateringPolicy_Reset(&state);
    state.lastWateringSeconds = 5000U;
    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);
}

static void test_auto_watering_disabled_blocks(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 10000U,
        .currentTick = 1000000UL
    };

    WATERING_POLICY_CONFIG config = s_defaultConfig;
    config.autoWateringEnabled = false;

    WateringPolicy_Reset(&state);
    state.lastWateringSeconds = 5000U;

    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &config);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);
}

static void test_unsynchronized_time_fallback(void) {
    WATERING_POLICY_STATE state;
    WATERING_POLICY_INPUT input = {
        .plantStatus = PLANT_STATUS_DRY_STRESS,
        .soilMoisturePermille = 250,
        .soilSensorHealth = SENSOR_HEALTH_OK,
        .tankLiquidDetected = true,
        .isMonitoring = true,
        .lastWateringResponse = WATERING_RESPONSE_OK,
        .nowSeconds = 0xFFFFFFFFU, /* unsynchronized! */
        .currentTick = 50000UL
    };

    WateringPolicy_Reset(&state);
    /* First time should allow */
    WATERING_DECISION decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_REQUEST, decision);

    /* Notify watering executed */
    WateringPolicy_NotifyWateringExecuted(&state, input.nowSeconds, input.currentTick);
    TEST_ASSERT_EQUAL_UINT(1U, state.dailyWateringCount);

    /* Try again after only 10000 ticks (< 180000 ticks) */
    input.currentTick = 60000UL;
    decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_BLOCKED, decision);

    /* Try again after 180000 ticks */
    input.currentTick = 50000UL + 180000UL;
    decision = WateringPolicy_Evaluate(&state, &input, &s_defaultConfig);
    TEST_ASSERT_EQUAL_INT(WATERING_DECISION_REQUEST, decision);
}

int main(void) {
    test_all_conditions_satisfied_requests_watering();
    test_condition1_not_dry_holds();
    test_condition2_sensor_error_blocks();
    test_condition3_tank_empty_blocks();
    test_condition4_interval_not_elapsed_blocks();
    test_condition5_last_watering_failed_blocks();
    test_condition6_daily_limit_reached_blocks();
    test_condition7_not_monitoring_blocks();
    test_auto_watering_disabled_blocks();
    test_unsynchronized_time_fallback();
    TEST_REPORT_AND_EXIT();
}
