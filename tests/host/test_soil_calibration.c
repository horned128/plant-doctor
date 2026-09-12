/** =================================================================*
 * @file   test_soil_calibration.c
 * @brief  土壌水分校正変換単体テスト
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"                  /* ホストテスト用アサーションマクロ */
#include "SoilCalibration.h"                /* 土壌水分校正変換API */

/** =================================================================*
 * @brief  校正値妥当性判定の検証
 * ================================================================= */
static void Test_CalibrationValidity(void) {
    /* 正常な校正値 */
    TEST_ASSERT_TRUE(SoilCalibration_IsValid(1000U, 3000U));

    /* 差がちょうど最小差分200 */
    TEST_ASSERT_TRUE(SoilCalibration_IsValid(1000U, 1200U));

    /* 差が最小差分未満（199） */
    TEST_ASSERT_FALSE(SoilCalibration_IsValid(1000U, 1199U));

    /* dry == wet */
    TEST_ASSERT_FALSE(SoilCalibration_IsValid(2000U, 2000U));

    /* wet < dry （逆転） */
    TEST_ASSERT_FALSE(SoilCalibration_IsValid(3000U, 1000U));
}

/** =================================================================*
 * @brief  相対湿り度変換およびクランプの検証
 * ================================================================= */
static void Test_CalibrationConversion(void) {
    uint16_t dry = 1000U;
    uint16_t wet = 3000U;

    /* dry以下は0にクランプ */
    TEST_ASSERT_EQUAL_INT(0, SoilCalibration_ToPermille(500U, dry, wet));
    TEST_ASSERT_EQUAL_INT(0, SoilCalibration_ToPermille(1000U, dry, wet));

    /* 中間値の計算 */
    TEST_ASSERT_EQUAL_INT(250, SoilCalibration_ToPermille(1500U, dry, wet));
    TEST_ASSERT_EQUAL_INT(500, SoilCalibration_ToPermille(2000U, dry, wet));
    TEST_ASSERT_EQUAL_INT(750, SoilCalibration_ToPermille(2500U, dry, wet));

    /* wet以上は1000にクランプ */
    TEST_ASSERT_EQUAL_INT(1000, SoilCalibration_ToPermille(3000U, dry, wet));
    TEST_ASSERT_EQUAL_INT(1000, SoilCalibration_ToPermille(3500U, dry, wet));
    TEST_ASSERT_EQUAL_INT(1000, SoilCalibration_ToPermille(4095U, dry, wet));

    /* 不当な校正値のときは0 */
    TEST_ASSERT_EQUAL_INT(0, SoilCalibration_ToPermille(2000U, 3000U, 1000U));
    TEST_ASSERT_EQUAL_INT(0, SoilCalibration_ToPermille(2000U, 2000U, 2000U));
}

/** =================================================================*
 * @brief  テストメインエントリ
 * @return 成功時0、失敗時1
 * ================================================================= */
int main(void) {
    TEST_RUN(Test_CalibrationValidity);
    TEST_RUN(Test_CalibrationConversion);

    TEST_REPORT_AND_EXIT();
}
