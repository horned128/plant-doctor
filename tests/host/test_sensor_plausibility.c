/** =================================================================*
 * @file   test_sensor_plausibility.c
 * @brief  センサー妥当性・整合性判定単体テスト
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"                  /* ホストテスト用アサーションマクロ */
#include "SensorPlausibility.h"             /* 妥当性判定API */

/** =================================================================*
 * @brief  全センサー正常ケースの検証
 * ================================================================= */
static void Test_AllHealthy(void) {
    SENSOR_PLAUSIBILITY_STATE state;
    SENSOR_PLAUSIBILITY_INPUT input;
    SENSOR_HEALTH_REPORT report;

    SensorPlausibility_Reset(&state);

    input.soilMoistureRaw = 2000U;
    input.soilMoistureValid = true;
    input.leafTemperatureCentiC = 2500;
    input.leafTemperatureValid = true;
    input.airTemperatureCentiC = 2400;
    input.airTemperatureValid = true;
    input.relativeHumidityCentiPercent = 5000U;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 300U;
    input.illuminanceValid = true;
    input.soilDryCalibration = 1000U;
    input.soilWetCalibration = 3000U;

    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);

    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.soilHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.leafHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.airHumHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.luxHealth);
}

/** =================================================================*
 * @brief  通信異常時の判定スキップ検証
 * ================================================================= */
static void Test_NoCommunication(void) {
    SENSOR_PLAUSIBILITY_STATE state;
    SENSOR_PLAUSIBILITY_INPUT input;
    SENSOR_HEALTH_REPORT report;

    SensorPlausibility_Reset(&state);

    /* 物理範囲外の値が入っていても通信無効ならNO_COMMUNICATION */
    input.soilMoistureRaw = 5000U;
    input.soilMoistureValid = false;
    input.leafTemperatureCentiC = 50000;
    input.leafTemperatureValid = false;
    input.airTemperatureCentiC = -9999;
    input.airTemperatureValid = false;
    input.relativeHumidityCentiPercent = 20000U;
    input.relativeHumidityValid = false;
    input.illuminanceRaw = 0U;
    input.illuminanceValid = false;
    input.soilDryCalibration = 1000U;
    input.soilWetCalibration = 3000U;

    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);

    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_NO_COMMUNICATION, report.soilHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_NO_COMMUNICATION, report.leafHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_NO_COMMUNICATION, report.airHumHealth);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_NO_COMMUNICATION, report.luxHealth);
}

/** =================================================================*
 * @brief  物理範囲外判定の検証
 * ================================================================= */
static void Test_OutOfRange(void) {
    SENSOR_PLAUSIBILITY_STATE state;
    SENSOR_PLAUSIBILITY_INPUT input;
    SENSOR_HEALTH_REPORT report;

    SensorPlausibility_Reset(&state);

    /* 正常初期値 */
    input.soilMoistureRaw = 2000U;
    input.soilMoistureValid = true;
    input.leafTemperatureCentiC = 2500;
    input.leafTemperatureValid = true;
    input.airTemperatureCentiC = 2400;
    input.airTemperatureValid = true;
    input.relativeHumidityCentiPercent = 5000U;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 300U;
    input.illuminanceValid = true;
    input.soilDryCalibration = 1000U;
    input.soilWetCalibration = 3000U;

    /* 気温範囲外 (>85℃) */
    input.airTemperatureCentiC = 8600;
    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OUT_OF_RANGE, report.airHumHealth);

    /* 湿度範囲外 (>100%) */
    input.airTemperatureCentiC = 2500;
    input.relativeHumidityCentiPercent = 10001U;
    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OUT_OF_RANGE, report.airHumHealth);

    /* 葉温範囲外 (>120℃) */
    input.relativeHumidityCentiPercent = 5000U;
    input.leafTemperatureCentiC = 12500;
    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OUT_OF_RANGE, report.leafHealth);
}

/** =================================================================*
 * @brief  葉温－気温差の継続整合性異常判定（4回OK, 5回目で異常）
 * ================================================================= */
static void Test_TemperatureDeltaPersistence(void) {
    SENSOR_PLAUSIBILITY_STATE state;
    SENSOR_PLAUSIBILITY_INPUT input;
    SENSOR_HEALTH_REPORT report;
    int i;

    SensorPlausibility_Reset(&state);

    input.soilMoistureRaw = 2000U;
    input.soilMoistureValid = true;
    input.airTemperatureCentiC = 2000;      /* 20.00 ℃ */
    input.airTemperatureValid = true;
    input.leafTemperatureCentiC = 4500;     /* 45.00 ℃ (差 25.00 ℃ > 20.00 ℃) */
    input.leafTemperatureValid = true;
    input.relativeHumidityCentiPercent = 5000U;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 300U;
    input.illuminanceValid = true;
    input.soilDryCalibration = 1000U;
    input.soilWetCalibration = 3000U;

    /* 1〜4サンプル目: まだ異常と断定しない (OK) */
    for (i = 1; i <= 4; ++i) {
        SensorPlausibility_Evaluate(&state, &input);
        SensorPlausibility_GetReport(&state, &report);
        TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.leafHealth);
    }

    /* 5サンプル目: 継続したためINCONSISTENT */
    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_INCONSISTENT, report.leafHealth);
}

/** =================================================================*
 * @brief  照度0継続異常判定（DEMO5: 4回OK, 5回目で異常）
 * ================================================================= */
static void Test_LuxZeroPersistence(void) {
    SENSOR_PLAUSIBILITY_STATE state;
    SENSOR_PLAUSIBILITY_INPUT input;
    SENSOR_HEALTH_REPORT report;
    int i;

    SensorPlausibility_Reset(&state);

    input.soilMoistureRaw = 2000U;
    input.soilMoistureValid = true;
    input.leafTemperatureCentiC = 2000;
    input.leafTemperatureValid = true;
    input.airTemperatureCentiC = 2000;
    input.airTemperatureValid = true;
    input.relativeHumidityCentiPercent = 5000U;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 0U;              /* 照度センサを覆った */
    input.illuminanceValid = true;
    input.soilDryCalibration = 1000U;
    input.soilWetCalibration = 3000U;

    for (i = 1; i <= 4; ++i) {
        SensorPlausibility_Evaluate(&state, &input);
        SensorPlausibility_GetReport(&state, &report);
        TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.luxHealth);
    }

    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_INCONSISTENT, report.luxHealth);
    /* 他のセンサーはOK */
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.airHumHealth);
}

/** =================================================================*
 * @brief  土壌水分急変の検出検証
 * ================================================================= */
static void Test_SoilStepChange(void) {
    SENSOR_PLAUSIBILITY_STATE state;
    SENSOR_PLAUSIBILITY_INPUT input;
    SENSOR_HEALTH_REPORT report;

    SensorPlausibility_Reset(&state);

    input.soilMoistureRaw = 2000U;
    input.soilMoistureValid = true;
    input.leafTemperatureCentiC = 2000;
    input.leafTemperatureValid = true;
    input.airTemperatureCentiC = 2000;
    input.airTemperatureValid = true;
    input.relativeHumidityCentiPercent = 5000U;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 300U;
    input.illuminanceValid = true;
    input.soilDryCalibration = 1000U;
    input.soilWetCalibration = 3000U;

    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OK, report.soilHealth);

    /* センサ抜去による急変 (2000 -> 300, 差1700 > 1500) */
    input.soilMoistureRaw = 300U;
    SensorPlausibility_Evaluate(&state, &input);
    SensorPlausibility_GetReport(&state, &report);
    TEST_ASSERT_EQUAL_INT(SENSOR_HEALTH_OUT_OF_RANGE, report.soilHealth);
}

/** =================================================================*
 * @brief  テストメインエントリ
 * @return 成功時0、失敗時1
 * ================================================================= */
int main(void) {
    TEST_RUN(Test_AllHealthy);
    TEST_RUN(Test_NoCommunication);
    TEST_RUN(Test_OutOfRange);
    TEST_RUN(Test_TemperatureDeltaPersistence);
    TEST_RUN(Test_LuxZeroPersistence);
    TEST_RUN(Test_SoilStepChange);

    TEST_REPORT_AND_EXIT();
}
