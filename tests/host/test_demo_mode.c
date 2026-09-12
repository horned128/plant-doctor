/** =================================================================*
 * @file   test_demo_mode.c
 * @brief  デモモード基盤単体テスト (P8-1)
 * ================================================================= */
#include "assert_helper.h"
#include "DemoMode.h"

static void test_initial_state_inactive(void) {
    DEMO_MODE_STATE state;
    DemoMode_Reset(&state);
    TEST_ASSERT_FALSE(DemoMode_IsActive(&state));
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_OFF, DemoMode_GetScenario(&state));
}

static void test_sw1_long_press_toggles_mode(void) {
    DEMO_MODE_STATE state;
    DemoMode_Reset(&state);

    /* Press SW1 for 299 ticks (not enough for 300 ticks) */
    for (uint16_t i = 0; i < 299U; ++i) {
        DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    }
    TEST_ASSERT_FALSE(DemoMode_IsActive(&state));

    /* 300th tick -> long press fires! */
    DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    TEST_ASSERT_TRUE(DemoMode_IsActive(&state));
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_1_NORMAL, DemoMode_GetScenario(&state));

    /* Release button */
    DemoMode_ProcessSwitch1Tick(&state, false, 300U);
    TEST_ASSERT_TRUE(DemoMode_IsActive(&state));

    /* Another 300 ticks long press -> toggles OFF */
    for (uint16_t i = 0; i < 300U; ++i) {
        DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    }
    TEST_ASSERT_FALSE(DemoMode_IsActive(&state));
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_OFF, DemoMode_GetScenario(&state));
}

static void test_sw1_short_press_cycles_scenarios(void) {
    DEMO_MODE_STATE state;
    DemoMode_Reset(&state);
    DemoMode_SetActive(&state, true);
    DemoMode_SetScenario(&state, DEMO_SCENARIO_1_NORMAL);

    /* Short press SW1 (5 ticks < 300) then release */
    for (uint16_t i = 0; i < 5U; ++i) {
        DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    }
    DemoMode_ProcessSwitch1Tick(&state, false, 300U);
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_2_HEAT_STRESS, DemoMode_GetScenario(&state));

    /* Another short press -> DEMO3 */
    for (uint16_t i = 0; i < 5U; ++i) {
        DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    }
    DemoMode_ProcessSwitch1Tick(&state, false, 300U);
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_3_DRY_STRESS, DemoMode_GetScenario(&state));

    /* DEMO4 */
    DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    DemoMode_ProcessSwitch1Tick(&state, false, 300U);
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_4_WATERING_FAILED, DemoMode_GetScenario(&state));

    /* DEMO5 */
    DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    DemoMode_ProcessSwitch1Tick(&state, false, 300U);
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_5_SENSOR_ERROR, DemoMode_GetScenario(&state));

    /* Cycles back to DEMO1 */
    DemoMode_ProcessSwitch1Tick(&state, true, 300U);
    DemoMode_ProcessSwitch1Tick(&state, false, 300U);
    TEST_ASSERT_EQUAL_INT(DEMO_SCENARIO_1_NORMAL, DemoMode_GetScenario(&state));
}

static void test_scenario_offsets_application(void) {
    DEMO_MODE_STATE state;
    DemoMode_Reset(&state);

    int16_t leafTemp = 2500;
    int16_t soilPermille = 500;
    int32_t lux = 500;
    bool tankLiquid = true;
    SENSOR_HEALTH soilHealth = SENSOR_HEALTH_OK;

    /* Inactive mode: no modification */
    DemoMode_ApplyOffsets(&state, &leafTemp, &soilPermille, &lux, &tankLiquid, &soilHealth);
    TEST_ASSERT_EQUAL_INT(2500, leafTemp);
    TEST_ASSERT_EQUAL_INT(500, soilPermille);
    TEST_ASSERT_EQUAL_INT(500, lux);
    TEST_ASSERT_TRUE(tankLiquid);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, soilHealth);

    /* DEMO2: Heat Stress (+2.00 C, 2x lux) */
    DemoMode_SetActive(&state, true);
    DemoMode_SetScenario(&state, DEMO_SCENARIO_2_HEAT_STRESS);
    DemoMode_ApplyOffsets(&state, &leafTemp, &soilPermille, &lux, &tankLiquid, &soilHealth);
    TEST_ASSERT_EQUAL_INT(2700, leafTemp); /* 2500 + 200 = 2700 */
    TEST_ASSERT_EQUAL_INT(1000, lux);      /* 500 * 2 = 1000 */

    /* DEMO3: Dry Stress (-350 permille) */
    leafTemp = 2500;
    soilPermille = 500;
    lux = 500;
    DemoMode_SetScenario(&state, DEMO_SCENARIO_3_DRY_STRESS);
    DemoMode_ApplyOffsets(&state, &leafTemp, &soilPermille, &lux, &tankLiquid, &soilHealth);
    TEST_ASSERT_EQUAL_INT(150, soilPermille); /* 500 - 350 = 150 < 300 dry threshold! */

    /* DEMO4: Watering Failed (tank empty) */
    DemoMode_SetScenario(&state, DEMO_SCENARIO_4_WATERING_FAILED);
    DemoMode_ApplyOffsets(&state, &leafTemp, &soilPermille, &lux, &tankLiquid, &soilHealth);
    TEST_ASSERT_FALSE(tankLiquid);

    /* DEMO5: Soil Sensor Error (sensor health out of range) */
    DemoMode_SetScenario(&state, DEMO_SCENARIO_5_SENSOR_ERROR);
    DemoMode_ApplyOffsets(&state, &leafTemp, &soilPermille, &lux, &tankLiquid, &soilHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OUT_OF_RANGE, soilHealth);

    /* Deactivate clears everything */
    DemoMode_SetActive(&state, false);
    soilHealth = SENSOR_HEALTH_OK;
    DemoMode_ApplyOffsets(&state, &leafTemp, &soilPermille, &lux, &tankLiquid, &soilHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, soilHealth);
}

int main(void) {
    test_initial_state_inactive();
    test_sw1_long_press_toggles_mode();
    test_sw1_short_press_cycles_scenarios();
    test_scenario_offsets_application();
    TEST_REPORT_AND_EXIT();
}
