/** =================================================================*
 * @file   test_watering_response.c
 * @brief  水やり後自己診断・土壌劣化推定単体テスト (P6-1, P6-2)
 * ================================================================= */
#include "assert_helper.h"
#include "WateringResponse.h"

static WATERING_RESPONSE_CONFIG s_config = {
    .immediateSeconds = 30U,
    .delayedSeconds = 180U,
    .tempDeltaSeconds = 300U,
    .timeoutSeconds = 600U,
    .immediateTicks = 3000UL,
    .delayedTicks = 18000UL,
    .tempDeltaTicks = 30000UL,
    .timeoutTicks = 60000UL,
    .minSoilRecoveryPermille = 50,
    .minTempDropCentic = 30,
    .degradeRequiredCycles = 3U,
    .degradeMinGainPermille = 80
};

static void test_normal_watering_success(void) {
    WATERING_RESPONSE_STATE state;
    PUMP_WATERING_EVENT event = {
        .startTimeSeconds = 1000U,
        .onDurationTicks = 200U,
        .soilMoistureRawBefore = 1000U,
        .soilMoisturePermilleBefore = 200,
        .leafAirDeltaBefore = 200,
        .stopReason = (uint8_t)PUMP_STOP_REASON_MAX_ON_TIME,
        .tankLiquidAtStart = true,
        .automatic = true
    };

    WateringResponse_Reset(&state);
    WateringResponse_NotifyWatering(&state, &event, 100000UL);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_MEASURING, WateringResponse_GetResult(&state));

    /* After 180s: soil moisture recovered to 350 (+150 gain) */
    WATERING_RESPONSE_INPUT input1 = {
        .soilMoisturePermille = 350,
        .leafAirDeltaCentic = 200,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 1000U + 180U,
        .currentTick = 100000UL + 18000UL
    };
    WateringResponse_Update(&state, &input1, &s_config);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_MEASURING, WateringResponse_GetResult(&state));

    /* After 300s: leaf temperature delta dropped to 120 (+80 drop) */
    WATERING_RESPONSE_INPUT input2 = {
        .soilMoisturePermille = 350,
        .leafAirDeltaCentic = 120,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 1000U + 300U,
        .currentTick = 100000UL + 30000UL
    };
    WateringResponse_Update(&state, &input2, &s_config);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_OK, WateringResponse_GetResult(&state));
    TEST_ASSERT_FALSE(WateringResponse_DidNotCoolLeaf(&state));
    TEST_ASSERT_FALSE(WateringResponse_IsSoilDegraded(&state));
}

static void test_soil_recovered_but_leaf_not_cooling(void) {
    WATERING_RESPONSE_STATE state;
    PUMP_WATERING_EVENT event = {
        .startTimeSeconds = 1000U,
        .onDurationTicks = 200U,
        .soilMoistureRawBefore = 1000U,
        .soilMoisturePermilleBefore = 200,
        .leafAirDeltaBefore = 200,
        .stopReason = (uint8_t)PUMP_STOP_REASON_MAX_ON_TIME,
        .tankLiquidAtStart = true,
        .automatic = true
    };

    WateringResponse_Reset(&state);
    WateringResponse_NotifyWatering(&state, &event, 100000UL);

    /* 180s */
    WATERING_RESPONSE_INPUT input1 = {
        .soilMoisturePermille = 350,
        .leafAirDeltaCentic = 200,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 1180U,
        .currentTick = 118000UL
    };
    WateringResponse_Update(&state, &input1, &s_config);

    /* 300s: leaf delta dropped only by 10 (200 - 190 = 10 < 30) */
    WATERING_RESPONSE_INPUT input2 = {
        .soilMoisturePermille = 350,
        .leafAirDeltaCentic = 190,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 1300U,
        .currentTick = 130000UL
    };
    WateringResponse_Update(&state, &input2, &s_config);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_OK, WateringResponse_GetResult(&state));
    TEST_ASSERT_TRUE(WateringResponse_DidNotCoolLeaf(&state)); /* Root uptake failure candidate */
}

static void test_watering_failed_no_moisture_recovery(void) {
    WATERING_RESPONSE_STATE state;
    PUMP_WATERING_EVENT event = {
        .startTimeSeconds = 1000U,
        .onDurationTicks = 200U,
        .soilMoistureRawBefore = 1000U,
        .soilMoisturePermilleBefore = 200,
        .leafAirDeltaBefore = 200,
        .stopReason = (uint8_t)PUMP_STOP_REASON_MAX_ON_TIME,
        .tankLiquidAtStart = true,
        .automatic = true
    };

    WateringResponse_Reset(&state);
    WateringResponse_NotifyWatering(&state, &event, 100000UL);

    /* 180s: soil moisture only increased by 10 (210 - 200 = 10 < 50) */
    WATERING_RESPONSE_INPUT input = {
        .soilMoisturePermille = 210,
        .leafAirDeltaCentic = 200,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 1180U,
        .currentTick = 118000UL
    };
    WateringResponse_Update(&state, &input, &s_config);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_FAILED, WateringResponse_GetResult(&state));

    /* Clear failure */
    WateringResponse_ClearFailure(&state);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_IDLE, WateringResponse_GetResult(&state));
}

static void test_boundary_exact_minimum_recovery(void) {
    WATERING_RESPONSE_STATE state;
    PUMP_WATERING_EVENT event = {
        .startTimeSeconds = 1000U,
        .soilMoisturePermilleBefore = 200,
        .leafAirDeltaBefore = 150,
    };

    WateringResponse_Reset(&state);
    WateringResponse_NotifyWatering(&state, &event, 100000UL);

    /* Exactly +50 gain (250 - 200 = 50 == minSoilRecoveryPermille) */
    WATERING_RESPONSE_INPUT input = {
        .soilMoisturePermille = 250,
        .leafAirDeltaCentic = 150,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 1180U,
        .currentTick = 118000UL
    };
    WateringResponse_Update(&state, &input, &s_config);
    /* Should NOT fail! */
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_MEASURING, WateringResponse_GetResult(&state));
}

static void test_sensor_invalid_timeout_reverts_to_idle(void) {
    WATERING_RESPONSE_STATE state;
    PUMP_WATERING_EVENT event = {
        .startTimeSeconds = 1000U,
        .soilMoisturePermilleBefore = 200,
        .leafAirDeltaBefore = 150,
    };

    WateringResponse_Reset(&state);
    WateringResponse_NotifyWatering(&state, &event, 100000UL);

    /* Soil sensor invalid through timeout (600s) */
    WATERING_RESPONSE_INPUT input = {
        .soilMoisturePermille = 200,
        .leafAirDeltaCentic = 150,
        .soilMoistureValid = false, /* invalid! */
        .leafAirDeltaValid = false,
        .nowSeconds = 1600U,
        .currentTick = 160000UL
    };
    WateringResponse_Update(&state, &input, &s_config);
    /* Should return to IDLE, NOT FAILED */
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_IDLE, WateringResponse_GetResult(&state));
}

static void test_soil_degradation_three_cycle_persistence(void) {
    WATERING_RESPONSE_STATE state;
    PUMP_WATERING_EVENT event = {
        .startTimeSeconds = 1000U,
        .soilMoisturePermilleBefore = 200,
        .leafAirDeltaBefore = 100,
    };

    WateringResponse_Reset(&state);

    /* Cycle 1: gain is 60 (>= 50 min recovery, but < 80 degrade threshold) */
    WateringResponse_NotifyWatering(&state, &event, 100000UL);
    WATERING_RESPONSE_INPUT input1 = {
        .soilMoisturePermille = 260,
        .leafAirDeltaCentic = 70,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 1180U,
        .currentTick = 118000UL
    };
    WateringResponse_Update(&state, &input1, &s_config);
    input1.nowSeconds = 1300U;
    input1.currentTick = 130000UL;
    WateringResponse_Update(&state, &input1, &s_config);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_OK, WateringResponse_GetResult(&state));
    TEST_ASSERT_FALSE(WateringResponse_IsSoilDegraded(&state)); /* 1 cycle not enough */

    /* Cycle 2: gain is 65 (< 80) */
    event.startTimeSeconds = 5000U;
    WateringResponse_NotifyWatering(&state, &event, 500000UL);
    WATERING_RESPONSE_INPUT input2 = {
        .soilMoisturePermille = 265,
        .leafAirDeltaCentic = 70,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 5180U,
        .currentTick = 518000UL
    };
    WateringResponse_Update(&state, &input2, &s_config);
    input2.nowSeconds = 5300U;
    input2.currentTick = 530000UL;
    WateringResponse_Update(&state, &input2, &s_config);
    TEST_ASSERT_FALSE(WateringResponse_IsSoilDegraded(&state)); /* 2 cycles not enough */

    /* Cycle 3: gain is 70 (< 80) -> 3rd consecutive cycle! */
    event.startTimeSeconds = 10000U;
    WateringResponse_NotifyWatering(&state, &event, 1000000UL);
    WATERING_RESPONSE_INPUT input3 = {
        .soilMoisturePermille = 270,
        .leafAirDeltaCentic = 70,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 10180U,
        .currentTick = 1018000UL
    };
    WateringResponse_Update(&state, &input3, &s_config);
    input3.nowSeconds = 10300U;
    input3.currentTick = 1030000UL;
    WateringResponse_Update(&state, &input3, &s_config);
    TEST_ASSERT_TRUE(WateringResponse_IsSoilDegraded(&state)); /* 3 cycles -> Soil Degraded! */

    /* Cycle 4: normal recovery (gain 150 >= 80) -> resets degradation */
    event.startTimeSeconds = 15000U;
    WateringResponse_NotifyWatering(&state, &event, 1500000UL);
    WATERING_RESPONSE_INPUT input4 = {
        .soilMoisturePermille = 350,
        .leafAirDeltaCentic = 70,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 15180U,
        .currentTick = 1518000UL
    };
    WateringResponse_Update(&state, &input4, &s_config);
    input4.nowSeconds = 15300U;
    input4.currentTick = 1530000UL;
    WateringResponse_Update(&state, &input4, &s_config);
    TEST_ASSERT_FALSE(WateringResponse_IsSoilDegraded(&state)); /* Recovered from degradation */
}

static void test_unsynchronized_time_ticks_only(void) {
    WATERING_RESPONSE_STATE state;
    PUMP_WATERING_EVENT event = {
        .startTimeSeconds = 0xFFFFFFFFU, /* unsynchronized */
        .soilMoisturePermilleBefore = 200,
        .leafAirDeltaBefore = 100,
    };

    WateringResponse_Reset(&state);
    WateringResponse_NotifyWatering(&state, &event, 5000UL);

    WATERING_RESPONSE_INPUT input = {
        .soilMoisturePermille = 350,
        .leafAirDeltaCentic = 60,
        .soilMoistureValid = true,
        .leafAirDeltaValid = true,
        .nowSeconds = 0xFFFFFFFFU,
        .currentTick = 5000UL + 18000UL /* 180s */
    };
    WateringResponse_Update(&state, &input, &s_config);

    input.currentTick = 5000UL + 30000UL; /* 300s */
    WateringResponse_Update(&state, &input, &s_config);
    TEST_ASSERT_EQUAL_INT(WATERING_RESPONSE_OK, WateringResponse_GetResult(&state));
    TEST_ASSERT_FALSE(WateringResponse_DidNotCoolLeaf(&state));
}

int main(void) {
    test_normal_watering_success();
    test_soil_recovered_but_leaf_not_cooling();
    test_watering_failed_no_moisture_recovery();
    test_boundary_exact_minimum_recovery();
    test_sensor_invalid_timeout_reverts_to_idle();
    test_soil_degradation_three_cycle_persistence();
    test_unsynchronized_time_ticks_only();
    TEST_REPORT_AND_EXIT();
}
