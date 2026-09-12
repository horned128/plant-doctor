/** =================================================================*
 * @file   test_lcd_ui.c
 * @brief  LCD表示・画面割り付けの単体テスト (P4-4)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "assert_helper.h"
#include "LcdUi.h"
#include "Lcd.h"
#include <string.h>

/* LCDドライバスタブ */
static char s_lcdMockLine1[17];
static char s_lcdMockLine2[17];
static bool s_lcdMockInitSuccess = true;

void Lcd_PeripheralInit(void) {}
LCD_STATUS Lcd_Init(void) {
    return s_lcdMockInitSuccess ? LCD_STATUS_OK : LCD_STATUS_TRANSFER_TIMEOUT;
}
LCD_STATUS Lcd_Draw(uint8_t position, const char *text) {
    char *dest = (position == LCD_START_OF_FIRST_LINE) ? s_lcdMockLine1 : s_lcdMockLine2;
    uint8_t i;
    for (i = 0U; (i < 16U) && (text[i] != '\0'); ++i) {
        dest[i] = text[i];
    }
    dest[i] = '\0';
    return LCD_STATUS_OK;
}
LCD_STATUS Lcd_ClearDisplay(void) { return LCD_STATUS_OK; }
LCD_STATUS Lcd_DisplayOnOff(uint8_t display, uint8_t cursor, uint8_t cursorBlink) {
    (void)display; (void)cursor; (void)cursorBlink;
    return LCD_STATUS_OK;
}
void Lcd_BacklightOn(void) {}
void Lcd_BacklightOff(void) {}

/** =================================================================*
 * @brief  全PLANT_STATUSおよびセンサ異常文字列が16文字以内であること
 * ================================================================= */
static void Test_AllStatusStringsLength(void) {
    char line[32];
    int status;
    int sensor;

    for (status = 0; status <= (int)PLANT_STATUS_LEARNING; ++status) {
        if (status == (int)PLANT_STATUS_SENSOR_ERROR) {
            for (sensor = 0; sensor <= (int)DIAGNOSIS_FAILED_SENSOR_LUX; ++sensor) {
                memset(line, 0, sizeof(line));
                LcdUi_FormatStatus((PLANT_STATUS)status, (DIAGNOSIS_FAILED_SENSOR)sensor, line, sizeof(line));
                TEST_ASSERT_TRUE(strlen(line) <= 16U);
                TEST_ASSERT_TRUE(strlen(line) > 0U);
            }
        } else {
            memset(line, 0, sizeof(line));
            LcdUi_FormatStatus((PLANT_STATUS)status, DIAGNOSIS_FAILED_SENSOR_NONE, line, sizeof(line));
            TEST_ASSERT_TRUE(strlen(line) <= 16U);
            TEST_ASSERT_TRUE(strlen(line) > 0U);
        }
    }
}

/** =================================================================*
 * @brief  全SOIL_TREND文字列が16文字以内であること
 * ================================================================= */
static void Test_AllSoilTrendStringsLength(void) {
    char line[32];
    int trend;

    for (trend = 0; trend <= (int)SOIL_TREND_DEGRADATION; ++trend) {
        memset(line, 0, sizeof(line));
        LcdUi_FormatSoilTrend((SOIL_TREND)trend, line, sizeof(line));
        TEST_ASSERT_TRUE(strlen(line) <= 16U);
        TEST_ASSERT_TRUE(strlen(line) > 0U);
    }
}

/** =================================================================*
 * @brief  ストレススコア行の整形検証 (0..100, --, デモモード)
 * ================================================================= */
static void Test_StressLineFormatting(void) {
    char line[32];

    /* 通常時 */
    LcdUi_FormatStressLine(18U, false, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("STRESS:  18/100", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    LcdUi_FormatStressLine(72U, false, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("STRESS:  72/100", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    LcdUi_FormatStressLine(100U, false, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("STRESS: 100/100", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    LcdUi_FormatStressLine(PLANT_STRESS_UNKNOWN, false, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("STRESS:  --/100", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    /* デモモード時 (*) */
    LcdUi_FormatStressLine(72U, true, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("STRESS: 72/100 *", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    LcdUi_FormatStressLine(PLANT_STRESS_UNKNOWN, true, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("STRESS: --/100 *", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);
}

/** =================================================================*
 * @brief  葉温－気温差行の整形検証 (正・負・無効)
 * ================================================================= */
static void Test_LeafAirDeltaFormatting(void) {
    char line[32];

    /* 正の温度差 (+0.8℃) */
    LcdUi_FormatLeafAirDelta(80, true, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:+0.8C", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    /* 正の温度差 (+2.0℃, DEMO2) */
    LcdUi_FormatLeafAirDelta(200, true, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:+2.0C", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    /* 負の温度差 (-0.5℃) */
    LcdUi_FormatLeafAirDelta(-50, true, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:-0.5C", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    /* 大きな温度差 (+12.3℃) */
    LcdUi_FormatLeafAirDelta(1234, true, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:+12.3C", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);

    /* 無効時 */
    LcdUi_FormatLeafAirDelta(0, false, line, sizeof(line));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:--", line);
    TEST_ASSERT_TRUE(strlen(line) <= 16U);
}

/** =================================================================*
 * @brief  企画5デモの表示例の検証 (DEMO1〜DEMO5)
 * ================================================================= */
static void Test_ProjectDemosDisplay(void) {
    LCD_DIAGNOSIS_VIEW_DATA diag;
    PLANT_SENSOR_SNAPSHOT snap;
    char line1[32];
    char line2[32];

    memset(&snap, 0, sizeof(snap));

    /* DEMO1: 通常診断 -> Stress: 18/100 / Healthy / +0.8C / Stable */
    diag.stressScore = 18U;
    diag.status = PLANT_STATUS_HEALTHY;
    diag.failedSensor = DIAGNOSIS_FAILED_SENSOR_NONE;
    diag.leafAirTemperatureDelta = 80;
    diag.leafAirDeltaValid = true;
    diag.soilTrend = SOIL_TREND_STABLE;
    diag.isDemoMode = false;

    LcdUi_FormatPage(&diag, &snap, 0U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("STRESS:  18/100", line1);
    TEST_ASSERT_EQUAL_STRING("HEALTHY", line2);

    LcdUi_FormatPage(&diag, &snap, 1U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:+0.8C", line1);
    TEST_ASSERT_EQUAL_STRING("SOIL: STABLE", line2);

    /* DEMO2: 日照・熱ストレス -> Stress: 72/100 / Heat Stress / +2.0C / Stable */
    diag.stressScore = 72U;
    diag.status = PLANT_STATUS_HEAT_STRESS;
    diag.leafAirTemperatureDelta = 200;
    diag.soilTrend = SOIL_TREND_STABLE;

    LcdUi_FormatPage(&diag, &snap, 0U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("STRESS:  72/100", line1);
    TEST_ASSERT_EQUAL_STRING("HEAT STRESS", line2);

    LcdUi_FormatPage(&diag, &snap, 1U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:+2.0C", line1);
    TEST_ASSERT_EQUAL_STRING("SOIL: STABLE", line2);

    /* DEMO3: 乾燥ストレス・自律水やり -> Stress: 80/100 / Watering / +0.5C / Dry */
    diag.stressScore = 80U;
    diag.status = PLANT_STATUS_WATERING;
    diag.leafAirTemperatureDelta = 50;
    diag.soilTrend = SOIL_TREND_DRY;

    LcdUi_FormatPage(&diag, &snap, 0U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("STRESS:  80/100", line1);
    TEST_ASSERT_EQUAL_STRING("WATERING", line2);

    LcdUi_FormatPage(&diag, &snap, 1U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:+0.5C", line1);
    TEST_ASSERT_EQUAL_STRING("SOIL: DRY", line2);

    /* DEMO4: 水やり失敗 -> Stress: 80/100 / Watering Failed / +0.5C / Dry */
    diag.status = PLANT_STATUS_WATERING_FAILED;
    LcdUi_FormatPage(&diag, &snap, 0U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("STRESS:  80/100", line1);
    TEST_ASSERT_EQUAL_STRING("WATER FAILED", line2);

    /* DEMO5: センサー異常 -> Stress: 80/100 / Soil Sensor Error / +0.5C / Dry */
    diag.status = PLANT_STATUS_SENSOR_ERROR;
    diag.failedSensor = DIAGNOSIS_FAILED_SENSOR_SOIL;
    LcdUi_FormatPage(&diag, &snap, 0U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("STRESS:  80/100", line1);
    TEST_ASSERT_EQUAL_STRING("SOIL SENS ERR", line2);

    LcdUi_FormatPage(&diag, &snap, 1U, line1, line2, sizeof(line1));
    TEST_ASSERT_EQUAL_STRING("LEAF-AIR:+0.5C", line1);
    TEST_ASSERT_EQUAL_STRING("SOIL: DRY", line2);
}

/** =================================================================*
 * @brief  ページ2（生センサー値）の整形検証
 * ================================================================= */
static void Test_RawSensorsPage2(void) {
    PLANT_SENSOR_SNAPSHOT snap;
    char line1[32];
    char line2[32];

    snap.airTemperatureCentiC = 2500;
    snap.airTemperatureValid = true;
    snap.relativeHumidityCentiPercent = 5000U;
    snap.illuminanceRaw = 123U;
    snap.illuminanceValid = true;
    snap.tankLiquidDetected = true;

    LcdUi_FormatPage(NULL, &snap, 2U, line1, line2, sizeof(line1));
    TEST_ASSERT_TRUE(strlen(line1) <= 16U);
    TEST_ASSERT_TRUE(strlen(line2) <= 16U);
    TEST_ASSERT_EQUAL_STRING("AIR:+25.0 H:50%", line1);
    TEST_ASSERT_EQUAL_STRING("LUX:123 TANK:WET", line2);
}

/** =================================================================*
 * @brief  LCD初期化と描画API（LcdUi_ShowDiagnosisPage）の検証
 * ================================================================= */
static void Test_LcdUiShowDiagnosisPage(void) {
    LCD_DIAGNOSIS_VIEW_DATA diag;
    PLANT_SENSOR_SNAPSHOT snap;

    TEST_ASSERT_TRUE(LcdUi_Init());
    TEST_ASSERT_TRUE(LcdUi_IsReady());

    diag.stressScore = 72U;
    diag.status = PLANT_STATUS_HEAT_STRESS;
    diag.failedSensor = DIAGNOSIS_FAILED_SENSOR_NONE;
    diag.leafAirTemperatureDelta = 200;
    diag.leafAirDeltaValid = true;
    diag.soilTrend = SOIL_TREND_STABLE;
    diag.isDemoMode = false;

    memset(&snap, 0, sizeof(snap));

    /* ページ0描画 */
    TEST_ASSERT_TRUE(LcdUi_ShowDiagnosisPage(&diag, &snap, 0U));
    TEST_ASSERT_EQUAL_STRING("STRESS:  72/100 ", s_lcdMockLine1); /* 16文字パディング */
    TEST_ASSERT_EQUAL_STRING("HEAT STRESS     ", s_lcdMockLine2);
}

int main(void) {
    TEST_RUN(Test_AllStatusStringsLength);
    TEST_RUN(Test_AllSoilTrendStringsLength);
    TEST_RUN(Test_StressLineFormatting);
    TEST_RUN(Test_LeafAirDeltaFormatting);
    TEST_RUN(Test_ProjectDemosDisplay);
    TEST_RUN(Test_RawSensorsPage2);
    TEST_RUN(Test_LcdUiShowDiagnosisPage);
    TEST_REPORT_AND_EXIT();
}
