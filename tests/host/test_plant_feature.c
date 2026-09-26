/** =================================================================*
 * @file   test_plant_feature.c
 * @brief  特徴量抽出単体テスト
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"                  /* ホストテスト用アサーションマクロ */
#include "PlantFeature.h"                   /* 特徴量型とAPI */

/** =================================================================*
 * @brief  初期化状態および窓未充足時の検証
 * ================================================================= */
static void Test_InitialState(void) {
    PLANT_FEATURE_STATE state;
    PLANT_FEATURE_VECTOR vector;
    PLANT_FEATURE_INPUT input;

    PlantFeature_Reset(&state);
    PlantFeature_GetVector(&state, &vector);

    TEST_ASSERT_EQUAL_UINT(0U, vector.validMask);
    TEST_ASSERT_EQUAL_UINT(0U, vector.elapsedSinceWateringSeconds);

    /* 最初の1サンプルを入力 */
    input.soilMoisturePermille = 500;
    input.leafTemperatureCentiC = 2500;
    input.airTemperatureCentiC = 2400;
    input.relativeHumidityCentiPercent = 6000;
    input.illuminanceRaw = 300;
    input.soilMoistureValid = true;
    input.leafTemperatureValid = true;
    input.airTemperatureValid = true;
    input.relativeHumidityValid = true;
    input.illuminanceValid = true;
    input.wateringOccurred = false;

    PlantFeature_Update(&state, &input);
    PlantFeature_GetVector(&state, &vector);

    /* 1サンプル目でも移動平均は算出可能（count=1） */
    TEST_ASSERT_TRUE((vector.validMask & PLANT_FEATURE_VALID_SOIL_PERMILLE) != 0U);
    TEST_ASSERT_TRUE((vector.validMask & PLANT_FEATURE_VALID_SOIL_MA) != 0U);
    TEST_ASSERT_EQUAL_INT(500, vector.soilMoistureMovingAverage);
    TEST_ASSERT_EQUAL_INT(100, vector.leafAirTemperatureDelta); /* 25.00 - 24.00 = +1.00C */
    TEST_ASSERT_EQUAL_UINT(1U, vector.elapsedSinceWateringSeconds);

    /* 1分未経過なので1時間あたり変化率はまだ算出不可 */
    TEST_ASSERT_FALSE((vector.validMask & PLANT_FEATURE_VALID_SOIL_RATE) != 0U);
    TEST_ASSERT_FALSE((vector.validMask & PLANT_FEATURE_VALID_LEAF_RATE) != 0U);
}

/** =================================================================*
 * @brief  負の葉温－気温差および境界値の検証
 * ================================================================= */
static void Test_NegativeDeltaAndBoundaries(void) {
    PLANT_FEATURE_STATE state;
    PLANT_FEATURE_VECTOR vector;
    PLANT_FEATURE_INPUT input;

    PlantFeature_Reset(&state);

    input.soilMoisturePermille = 1000;
    input.leafTemperatureCentiC = 1850;     /* 18.50 ℃ (葉が蒸散で冷えている) */
    input.airTemperatureCentiC = 2300;      /* 23.00 ℃ */
    input.relativeHumidityCentiPercent = 5000;
    input.illuminanceRaw = 100;
    input.soilMoistureValid = true;
    input.leafTemperatureValid = true;
    input.airTemperatureValid = true;
    input.relativeHumidityValid = true;
    input.illuminanceValid = true;
    input.wateringOccurred = false;

    PlantFeature_Update(&state, &input);
    PlantFeature_GetVector(&state, &vector);

    /* 18.50 - 23.00 = -4.50C */
    TEST_ASSERT_EQUAL_INT(-450, vector.leafAirTemperatureDelta);
    TEST_ASSERT_TRUE((vector.validMask & PLANT_FEATURE_VALID_LEAF_AIR_DELTA) != 0U);
}

/** =================================================================*
 * @brief  無効サンプルの混入時の伝播検証
 * ================================================================= */
static void Test_InvalidSampleHandling(void) {
    PLANT_FEATURE_STATE state;
    PLANT_FEATURE_VECTOR vector;
    PLANT_FEATURE_INPUT input;

    PlantFeature_Reset(&state);

    /* 土壌水分のみ無効 */
    input.soilMoisturePermille = 0;
    input.soilMoistureValid = false;
    input.leafTemperatureCentiC = 2500;
    input.airTemperatureCentiC = 2500;
    input.leafTemperatureValid = true;
    input.airTemperatureValid = true;
    input.relativeHumidityCentiPercent = 4000;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 200;
    input.illuminanceValid = true;
    input.wateringOccurred = false;

    PlantFeature_Update(&state, &input);
    PlantFeature_GetVector(&state, &vector);

    /* 土壌水分はvalidMaskに含まれず、0として混入しない */
    TEST_ASSERT_FALSE((vector.validMask & PLANT_FEATURE_VALID_SOIL_PERMILLE) != 0U);
    TEST_ASSERT_FALSE((vector.validMask & PLANT_FEATURE_VALID_SOIL_MA) != 0U);
    /* 葉温－気温差は有効 */
    TEST_ASSERT_TRUE((vector.validMask & PLANT_FEATURE_VALID_LEAF_AIR_DELTA) != 0U);
    TEST_ASSERT_EQUAL_INT(0, vector.leafAirTemperatureDelta);
}

/** =================================================================*
 * @brief  給水イベント発生時の経過秒リセット検証
 * ================================================================= */
static void Test_WateringReset(void) {
    PLANT_FEATURE_STATE state;
    PLANT_FEATURE_VECTOR vector;
    PLANT_FEATURE_INPUT input;
    int i;

    PlantFeature_Reset(&state);

    input.soilMoisturePermille = 200;
    input.soilMoistureValid = true;
    input.leafTemperatureCentiC = 2000;
    input.airTemperatureCentiC = 2000;
    input.leafTemperatureValid = true;
    input.airTemperatureValid = true;
    input.relativeHumidityCentiPercent = 5000;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 100;
    input.illuminanceValid = true;
    input.wateringOccurred = false;

    for (i = 0; i < 10; ++i) {
        PlantFeature_Update(&state, &input);
    }
    PlantFeature_GetVector(&state, &vector);
    TEST_ASSERT_EQUAL_UINT(10U, vector.elapsedSinceWateringSeconds);

    /* 給水発生 */
    input.wateringOccurred = true;
    PlantFeature_Update(&state, &input);
    PlantFeature_GetVector(&state, &vector);
    TEST_ASSERT_EQUAL_UINT(0U, vector.elapsedSinceWateringSeconds);
}

/** =================================================================*
 * @brief  定常値および変化速度（低下・上昇）の検証
 * ================================================================= */
static void Test_RateCalculations(void) {
    PLANT_FEATURE_STATE state;
    PLANT_FEATURE_VECTOR vector;
    PLANT_FEATURE_INPUT input;
    int sec;

    PlantFeature_Reset(&state);

    input.soilMoistureValid = true;
    input.leafTemperatureValid = true;
    input.airTemperatureValid = true;
    input.relativeHumidityValid = true;
    input.illuminanceValid = true;
    input.wateringOccurred = false;
    input.airTemperatureCentiC = 2000;
    input.relativeHumidityCentiPercent = 5000;
    input.illuminanceRaw = 500;

    /* 最初の1分間: 土壌水分 800, 葉温 2000 */
    input.soilMoisturePermille = 800;
    input.leafTemperatureCentiC = 2000;
    for (sec = 0; sec < 60; ++sec) {
        PlantFeature_Update(&state, &input);
    }

    /* 次の1分間: 土壌水分 700 (低下), 葉温 2100 (上昇) */
    input.soilMoisturePermille = 700;
    input.leafTemperatureCentiC = 2100;
    for (sec = 0; sec < 60; ++sec) {
        PlantFeature_Update(&state, &input);
    }

    PlantFeature_GetVector(&state, &vector);

    /* 1分以上経過したので変化率が算出される */
    TEST_ASSERT_TRUE((vector.validMask & PLANT_FEATURE_VALID_SOIL_RATE) != 0U);
    TEST_ASSERT_TRUE((vector.validMask & PLANT_FEATURE_VALID_LEAF_RATE) != 0U);

    /* 土壌水分の低下速度: (800 - 700) * 60 / 1分 = 6000 ‰/h */
    TEST_ASSERT_EQUAL_INT(6000, vector.soilMoistureRatePerHour);
    /* 葉温の上昇速度: (2100 - 2000) * 60 / 1分 = 6000 centiC/h */
    TEST_ASSERT_EQUAL_INT(6000, vector.leafTemperatureRatePerHour);
}

/** =================================================================*
 * @brief  積算照度の積算および押し出し検証
 * ================================================================= */
static void Test_IlluminanceAccumulation(void) {
    PLANT_FEATURE_STATE state;
    PLANT_FEATURE_VECTOR vector;
    PLANT_FEATURE_INPUT input;
    int min;

    PlantFeature_Reset(&state);

    input.soilMoisturePermille = 500;
    input.soilMoistureValid = true;
    input.leafTemperatureCentiC = 2000;
    input.airTemperatureCentiC = 2000;
    input.leafTemperatureValid = true;
    input.airTemperatureValid = true;
    input.relativeHumidityCentiPercent = 5000;
    input.relativeHumidityValid = true;
    input.illuminanceRaw = 100;              /* 1分あたり100 */
    input.illuminanceValid = true;
    input.wateringOccurred = false;

    /* 60分経過（1時間）: 起動直後は24時間蓄積完了までマスクされていることを検証 */
    for (min = 0; min < 60; ++min) {
        int sec;
        for (sec = 0; sec < 60; ++sec) {
            PlantFeature_Update(&state, &input);
        }
    }

    PlantFeature_GetVector(&state, &vector);
    TEST_ASSERT_EQUAL_INT(0U, (vector.validMask & PLANT_FEATURE_VALID_ILLUMINANCE_ACCUM));
    TEST_ASSERT_EQUAL_INT(6000, vector.illuminanceAccumulated);

    /* 24時間蓄積完了（残り23時間分） */
    for (min = 60; min < (24 * 60); ++min) {
        int sec;
        for (sec = 0; sec < 60; ++sec) {
            PlantFeature_Update(&state, &input);
        }
    }

    PlantFeature_GetVector(&state, &vector);
    TEST_ASSERT_TRUE((vector.validMask & PLANT_FEATURE_VALID_ILLUMINANCE_ACCUM) != 0U);
    /* 24時間 * 60分 * 100 = 144000 */
    TEST_ASSERT_EQUAL_INT(144000, vector.illuminanceAccumulated);
}

/** =================================================================*
 * @brief  テストメインエントリ
 * @return 成功時0、失敗時1
 * ================================================================= */
int main(void) {
    TEST_RUN(Test_InitialState);
    TEST_RUN(Test_NegativeDeltaAndBoundaries);
    TEST_RUN(Test_InvalidSampleHandling);
    TEST_RUN(Test_WateringReset);
    TEST_RUN(Test_RateCalculations);
    TEST_RUN(Test_IlluminanceAccumulation);

    TEST_REPORT_AND_EXIT();
}
