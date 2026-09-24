/** =================================================================*
 * @file   LcdUi.c
 * @brief  LCDユーザーインターフェース
 * ================================================================= */
#include "LcdUi.h"                                          /* LcdUiのAPIと型定義 */
#include <stdio.h>                                          /* 標準Cの文字列整形API */
#include "Lcd.h"                                            /* LcdのAPIと型定義 */
#include "SwitchControl.h"                                  /* SwitchControlのAPIと型定義 */

static bool s_ready;                                        /**< モジュール内部状態 */

/** =================================================================*
 * @brief  LcdUi_WriteLine処理
 * @param[in] position 引数
 * @param[in] text 引数
 * @return 実行結果または取得値
 * ================================================================= */
static bool LcdUi_WriteLine(uint8_t position, const char *text) {
    char line[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
    LCD_STATUS status;
    uint8_t index = 0U;

    while (index < LCD_MOST_CHARACTERS_ON_A_LINE) {
        line[index] = ' ';
        ++index;
    }
    line[LCD_MOST_CHARACTERS_ON_A_LINE] = '\0';

    index = 0U;
    while ((index < LCD_MOST_CHARACTERS_ON_A_LINE) && (text[index] != '\0')) {
        line[index] = text[index];
        ++index;
    }

    status = Lcd_Draw(position, line);
    if (status != LCD_STATUS_OK) {
        return false;
    }
    return true;
}

/** =================================================================*
 * @brief  LCD周辺回路と表示器を初期化する。
 * @return 初期化成功時はtrue
 * ================================================================= */
static bool LcdUi_Initialize(void) {
    LCD_STATUS status;

    s_ready = false;
    Lcd_PeripheralInit();
    status = Lcd_Init();
    if (status == LCD_STATUS_OK) {
        status = Lcd_DisplayOnOff(LCD_DISPLAY_ON, LCD_CURSOR_OFF, LCD_CURSOR_BLINK_OFF);
    }
    if (status == LCD_STATUS_OK) {
        Lcd_BacklightOn();
        s_ready = true;
    } else {
        Lcd_BacklightOff();
    }
    return s_ready;
}


/** =================================================================*
 * @brief  LcdUi_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool LcdUi_Init(void) {
    return LcdUi_Initialize();
}

/** =================================================================*
 * @brief  LCDを再初期化して表示を復旧する。
 * @return 復旧成功時はtrue
 * ================================================================= */
bool LcdUi_Recover(void) {
    LCD_STATUS status = Lcd_Init();

    if (status == LCD_STATUS_OK) {
        status = Lcd_DisplayOnOff(LCD_DISPLAY_ON, LCD_CURSOR_OFF, LCD_CURSOR_BLINK_OFF);
    }
    if (status == LCD_STATUS_OK) {
        Lcd_BacklightOn();
        s_ready = true;
    }
    return s_ready;
}

/** =================================================================*
 * @brief  LcdUi_IsReady処理
 * @return 実行結果または取得値
 * ================================================================= */
bool LcdUi_IsReady(void) {
    return s_ready;
}

/** =================================================================*
 * @brief  LcdUi_ShowBoardTest処理
 * @return 実行結果または取得値
 * ================================================================= */
bool LcdUi_ShowBoardTest(void) {
    return s_ready &&
        LcdUi_WriteLine(LCD_START_OF_FIRST_LINE, "PLANT DOCTOR") &&
        LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, "BOARD TEST");
}

/** =================================================================*
 * @brief  LcdUi_ShowSwitch処理
 * @param[in] pressedMask 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool LcdUi_ShowSwitch(uint8_t pressedMask) {
    const char *message = "BOARD TEST";

    if ((pressedMask & SWITCH_CONTROL_PSW1) != 0U) {
        message = "SW1 PRESSED";
    } else if ((pressedMask & SWITCH_CONTROL_PSW2) != 0U) {
        message = "SW2 PRESSED";
    } else if ((pressedMask & SWITCH_CONTROL_PSW3) != 0U) {
        message = "SW3 PRESSED";
    } else if ((pressedMask & SWITCH_CONTROL_PSW4) != 0U) {
        message = "SW4 PRESSED";
    }

    return s_ready && LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, message);
}

/** =================================================================*
 * @brief  ポンプの動作状態またはメッセージをLCD2行目に表示する。
 * @param[in] message 表示メッセージ
 * @return LCD更新成功時はtrue
 * ================================================================= */
bool LcdUi_ShowPumpStatus(const char *message) {
    if ((message == 0) || !s_ready) {
        return false;
    }
    return LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, message);
}

/** =================================================================*
 * @brief  診断ステータス文字列を整形（16文字以内）
 * ================================================================= */
void LcdUi_FormatStatus(PLANT_STATUS status, DIAGNOSIS_FAILED_SENSOR failedSensor, char *line, uint8_t lineSize) {
    const char *str;
    if ((line == NULL) || (lineSize == 0U)) {
        return;
    }
    switch (status) {
        case PLANT_STATUS_HEALTHY:
            str = "HEALTHY";
            break;
        case PLANT_STATUS_DRY_STRESS:
            str = "DRY STRESS";
            break;
        case PLANT_STATUS_HEAT_STRESS:
            str = "HEAT STRESS";
            break;
        case PLANT_STATUS_LOW_LIGHT:
            str = "LOW LIGHT?";
            break;
        case PLANT_STATUS_ROOT_UPTAKE:
            str = "ROOT UPTAKE?";
            break;
        case PLANT_STATUS_WATERING:
            str = "WATERING";
            break;
        case PLANT_STATUS_WATERING_FAILED:
            str = "WATER FAILED";
            break;
        case PLANT_STATUS_SOIL_DEGRADATION:
            str = "SOIL DEGRADE?";
            break;
        case PLANT_STATUS_WARNING:
            str = "WARNING";
            break;
        case PLANT_STATUS_SENSOR_ERROR:
            switch (failedSensor) {
                case DIAGNOSIS_FAILED_SENSOR_SOIL:
                    str = "SOIL SENS ERR";
                    break;
                case DIAGNOSIS_FAILED_SENSOR_LEAF:
                    str = "LEAF SENS ERR";
                    break;
                case DIAGNOSIS_FAILED_SENSOR_AIR_HUM:
                    str = "AIR SENS ERR";
                    break;
                case DIAGNOSIS_FAILED_SENSOR_LUX:
                    str = "LUX SENS ERR";
                    break;
                default:
                    str = "SENSOR ERROR";
                    break;
            }
            break;
        case PLANT_STATUS_LEARNING:
            str = "LEARNING";
            break;
        default:
            str = "UNKNOWN";
            break;
    }
    (void)snprintf(line, (size_t)lineSize, "%s", str);
}

/** =================================================================*
 * @brief  ストレススコア行を整形（16文字以内）
 * ================================================================= */
void LcdUi_FormatStressLine(uint8_t stressScore, bool isDemoMode, char *line, uint8_t lineSize) {
    if ((line == NULL) || (lineSize == 0U)) {
        return;
    }
    if (stressScore == PLANT_STRESS_UNKNOWN) {
        if (isDemoMode) {
            (void)snprintf(line, (size_t)lineSize, "STRESS: --/100 *");
        } else {
            (void)snprintf(line, (size_t)lineSize, "STRESS:  --/100");
        }
    } else {
        unsigned int val = (unsigned int)stressScore;
        if (val > 100U) {
            val = 100U;
        }
        if (isDemoMode) {
            if (val >= 100U) {
                (void)snprintf(line, (size_t)lineSize, "STRESS: 100/100*");
            } else {
                (void)snprintf(line, (size_t)lineSize, "STRESS: %2u/100 *", val);
            }
        } else {
            if (val >= 100U) {
                (void)snprintf(line, (size_t)lineSize, "STRESS: 100/100");
            } else {
                (void)snprintf(line, (size_t)lineSize, "STRESS:  %2u/100", val);
            }
        }
    }
}

/** =================================================================*
 * @brief  葉温－気温差行を整形（16文字以内）
 * ================================================================= */
void LcdUi_FormatLeafAirDelta(int32_t deltaCentiC, bool valid, char *line, uint8_t lineSize) {
    if ((line == NULL) || (lineSize == 0U)) {
        return;
    }
    if (!valid) {
        (void)snprintf(line, (size_t)lineSize, "LEAF-AIR:--");
    } else {
        int32_t val = deltaCentiC;
        char sign = '+';
        if (val < 0) {
            sign = '-';
            val = -val;
        }
        (void)snprintf(line, (size_t)lineSize, "LEAF-AIR:%c%ld.%ldC",
                       sign,
                       (long)(val / 100L),
                       (long)((val % 100L) / 10L));
    }
}

/** =================================================================*
 * @brief  土壌傾向行を整形（16文字以内）
 * ================================================================= */
void LcdUi_FormatSoilTrend(SOIL_TREND trend, char *line, uint8_t lineSize) {
    const char *str;
    if ((line == NULL) || (lineSize == 0U)) {
        return;
    }
    switch (trend) {
        case SOIL_TREND_STABLE:
            str = "SOIL: STABLE";
            break;
        case SOIL_TREND_DRY:
            str = "SOIL: DRY";
            break;
        case SOIL_TREND_WET:
            str = "SOIL: WET";
            break;
        case SOIL_TREND_DEGRADATION:
            str = "SOIL: DEGRADE";
            break;
        case SOIL_TREND_UNKNOWN:
        default:
            str = "SOIL: UNKNOWN";
            break;
    }
    (void)snprintf(line, (size_t)lineSize, "%s", str);
}

/** =================================================================*
 * @brief  ページ番号に応じた2行の表示内容を整形
 * ================================================================= */
void LcdUi_FormatPage(const LCD_DIAGNOSIS_VIEW_DATA *diagData,
                      const PLANT_SENSOR_SNAPSHOT *snapshot,
                      uint8_t page,
                      char *line1,
                      char *line2,
                      uint8_t lineSize) {
    if ((line1 == NULL) || (line2 == NULL) || (lineSize == 0U)) {
        return;
    }

    switch (page % 3U) {
        case 0U:
            if (diagData != NULL) {
                LcdUi_FormatStressLine(diagData->stressScore, diagData->isDemoMode, line1, lineSize);
                LcdUi_FormatStatus(diagData->status, diagData->failedSensor, line2, lineSize);
            } else {
                (void)snprintf(line1, (size_t)lineSize, "STRESS:  --/100");
                (void)snprintf(line2, (size_t)lineSize, "HEALTHY");
            }
            break;

        case 1U:
            if (diagData != NULL) {
                LcdUi_FormatLeafAirDelta(diagData->leafAirTemperatureDelta, diagData->leafAirDeltaValid, line1, lineSize);
                LcdUi_FormatSoilTrend(diagData->soilTrend, line2, lineSize);
            } else {
                (void)snprintf(line1, (size_t)lineSize, "LEAF-AIR:--");
                (void)snprintf(line2, (size_t)lineSize, "SOIL: UNKNOWN");
            }
            break;

        case 2U:
        default:
            if (snapshot != NULL) {
                if (snapshot->airTemperatureValid) {
                    char sign = (snapshot->airTemperatureCentiC < 0) ? '-' : '+';
                    int32_t val = (snapshot->airTemperatureCentiC < 0) ? -snapshot->airTemperatureCentiC : snapshot->airTemperatureCentiC;
                    (void)snprintf(line1, (size_t)lineSize, "AIR:%c%02ld.%ld H:%2lu%%",
                                   sign, (long)(val / 100L), (long)((val % 100L) / 10L),
                                   (unsigned long)(snapshot->relativeHumidityCentiPercent / 100U));
                } else {
                    (void)snprintf(line1, (size_t)lineSize, "AIR:--    H:--%%");
                }

                if (snapshot->illuminanceValid) {
                    if (snapshot->illuminanceRaw >= 1000U) {
                        (void)snprintf(line2, (size_t)lineSize, "LUX:%lu TK:%s",
                                       (unsigned long)snapshot->illuminanceRaw,
                                       snapshot->tankLiquidDetected ? "WET" : "EMP");
                    } else {
                        (void)snprintf(line2, (size_t)lineSize, "LUX:%lu TANK:%s",
                                       (unsigned long)snapshot->illuminanceRaw,
                                       snapshot->tankLiquidDetected ? "WET" : "EMP");
                    }
                } else {
                    (void)snprintf(line2, (size_t)lineSize, "LUX:--  TANK:%s",
                                   snapshot->tankLiquidDetected ? "WET" : "EMP");
                }
            } else {
                (void)snprintf(line1, (size_t)lineSize, "AIR:--    H:--%%");
                (void)snprintf(line2, (size_t)lineSize, "LUX:--   TANK:EMP");
            }
            break;
    }
}

/** =================================================================*
 * @brief  診断結果とセンサー値を3ページで表示する。 (P4-4)
 * @param[in] diagData 診断表示データ
 * @param[in] snapshot センサースナップショット
 * @param[in] page     表示ページ番号
 * @return LCD更新成功時はtrue
 * ================================================================= */
bool LcdUi_ShowDiagnosisPage(const LCD_DIAGNOSIS_VIEW_DATA *diagData,
                             const PLANT_SENSOR_SNAPSHOT *snapshot,
                             uint8_t page) {
    char line1[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
    char line2[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];

    if (!s_ready) {
        return false;
    }

    LcdUi_FormatPage(diagData, snapshot, page, line1, line2, sizeof(line1));
    return LcdUi_WriteLine(LCD_START_OF_FIRST_LINE, line1) &&
           LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, line2);
}

/** =================================================================*
 * @brief  センサー値を表示（互換性ラッパー）
 * ================================================================= */
bool LcdUi_ShowSensorPage(const PLANT_SENSOR_SNAPSHOT *snapshot, uint8_t page) {
    return LcdUi_ShowDiagnosisPage(NULL, snapshot, page);
}

/** =================================================================*
 * @brief  エラー表示行を整形（各行16文字以内）
 * ================================================================= */
void LcdUi_FormatError(PLANT_DOCTOR_ERROR error, char *line1, char *line2, uint8_t lineSize) {
    const char *message;

    if ((line1 == NULL) || (line2 == NULL) || (lineSize == 0U)) {
        return;
    }

    switch (error) {
        case PLANT_DOCTOR_ERROR_POWER:
            message = "ERROR POWER";
            break;
        case PLANT_DOCTOR_ERROR_TIMER:
            message = "ERROR TIMER";
            break;
        case PLANT_DOCTOR_ERROR_SWITCH:
            message = "ERROR SWITCH";
            break;
        case PLANT_DOCTOR_ERROR_LCD_INIT:
        case PLANT_DOCTOR_ERROR_LCD_IO:
            message = "ERROR LCD";
            break;
        case PLANT_DOCTOR_ERROR_TICK_OVERFLOW:
            message = "ERROR TIMING";
            break;
        case PLANT_DOCTOR_ERROR_SENSOR_INTERFACE:
            message = "ERROR SENSOR";
            break;
        case PLANT_DOCTOR_ERROR_STORAGE_INTERFACE:
            message = "ERROR STORAGE";
            break;
        case PLANT_DOCTOR_ERROR_AI:
            message = "ERROR AI";
            break;
        case PLANT_DOCTOR_ERROR_ACTUATOR:
            message = "ERROR PUMP";
            break;
        case PLANT_DOCTOR_ERROR_NONE:
        default:
            message = "ERROR UNKNOWN";
            break;
    }

    (void)snprintf(line1, (size_t)lineSize, "PLANT DOCTOR");
    (void)snprintf(line2, (size_t)lineSize, "%s", message);
}

/** =================================================================*
 * @brief  LcdUi_ShowError処理
 * @param[in] error 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool LcdUi_ShowError(PLANT_DOCTOR_ERROR error) {
    char line1[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
    char line2[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];

    if (!s_ready) {
        (void)LcdUi_Initialize();
    }
    if (!s_ready) {
        return false;
    }

    LcdUi_FormatError(error, line1, line2, sizeof(line1));

    return LcdUi_WriteLine(LCD_START_OF_FIRST_LINE, line1) &&
           LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, line2);
}
