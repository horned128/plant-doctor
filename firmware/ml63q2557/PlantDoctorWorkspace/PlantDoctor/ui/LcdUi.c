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
 * @brief  摂氏100分の1単位の温度をLCD用文字列へ整形する。
 * @param[out] text 整形結果バッファ
 * @param[in] textSize バッファサイズ
 * @param[in] temperatureCentiC 摂氏100分の1単位の温度
 * ================================================================= */
static void LcdUi_FormatTemperature(char *text, uint8_t textSize, int16_t temperatureCentiC) {
    int32_t value = temperatureCentiC;
    char sign = '+';

    if (value < 0L) {
        sign = '-';
        value = -value;
    }
    (void)snprintf(text, textSize, "%c%02ld.%02ldC", sign, value / 100L, value % 100L);
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
 * @brief  最新のセンサー値を3ページで表示する。
 * @param[in] snapshot センサースナップショット
 * @param[in] page 表示ページ番号
 * @return LCD更新成功時はtrue
 * ================================================================= */
bool LcdUi_ShowSensorPage(const PLANT_SENSOR_SNAPSHOT *snapshot, uint8_t page) {
    char line1[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
    char line2[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
    char temperature[12];

    if ((snapshot == 0) || !s_ready) {
        return false;
    }

    switch (page % 3U) {
        case 0U:
            if (snapshot->airTemperatureValid) {
                LcdUi_FormatTemperature(temperature, sizeof(temperature), snapshot->airTemperatureCentiC);
                (void)snprintf(line1, sizeof(line1), "AIR:%s", temperature);
                (void)snprintf(line2, sizeof(line2), "HUM:%lu.%02lu%%",
                    (unsigned long)(snapshot->relativeHumidityCentiPercent / 100U),
                    (unsigned long)(snapshot->relativeHumidityCentiPercent % 100U));
            } else {
                (void)snprintf(line1, sizeof(line1), "AIR:--");
                (void)snprintf(line2, sizeof(line2), "HUM:--");
            }
            break;

        case 1U:
            if (snapshot->leafTemperatureValid) {
                LcdUi_FormatTemperature(temperature, sizeof(temperature), snapshot->leafTemperatureCentiC);
                (void)snprintf(line1, sizeof(line1), "LEAF:%s", temperature);
            } else {
                (void)snprintf(line1, sizeof(line1), "LEAF:--");
            }
            if (snapshot->soilMoistureValid) {
                (void)snprintf(line2, sizeof(line2), "SOIL:%4u", snapshot->soilMoistureRaw);
            } else {
                (void)snprintf(line2, sizeof(line2), "SOIL:--");
            }
            break;

        case 2U:
        default:
            if (snapshot->illuminanceValid) {
                (void)snprintf(line1, sizeof(line1), "LUX:%lu.%02lu",
                    (unsigned long)(snapshot->illuminanceCentiLux / 100UL),
                    (unsigned long)(snapshot->illuminanceCentiLux % 100UL));
            } else {
                (void)snprintf(line1, sizeof(line1), "LUX:--");
            }
            (void)snprintf(line2, sizeof(line2), "TANK:%s", snapshot->tankLiquidDetected ? "WET" : "EMPTY");
            break;
    }

    return LcdUi_WriteLine(LCD_START_OF_FIRST_LINE, line1) && LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, line2);
}

/** =================================================================*
 * @brief  LcdUi_ShowError処理
 * @param[in] error 引数
 * @return 実行結果または取得値
 * ================================================================= */
bool LcdUi_ShowError(PLANT_DOCTOR_ERROR error) {
    const char *message;

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
        case PLANT_DOCTOR_ERROR_NONE:
        default:
            message = "ERROR UNKNOWN";
            break;
    }

    return s_ready &&
        LcdUi_WriteLine(LCD_START_OF_FIRST_LINE, "PLANT DOCTOR") &&
        LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, message);
}
