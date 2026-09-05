/** =================================================================*
 * @file   SensorDiagnostic.c
 * @brief  センサー診断アプリケーション
 * ================================================================= */
#include "SensorDiagnostic.h"                               /* SensorDiagnosticのAPIと型定義 */
#include <stdio.h>                                          /* 標準Cの書式出力 */
#include <string.h>                                         /* 標準Cのメモリ操作 */
#include "Board.h"                                          /* BoardのAPIと型定義 */
#include "Lcd.h"                                            /* LcdのAPIと型定義 */
#include "LedControl.h"                                     /* LedControlのAPIと型定義 */
#include "SwitchControl.h"                                  /* SwitchControlのAPIと型定義 */
#include "TimeControl.h"                                    /* 時間待ちAPI */

#define DIAGNOSTIC_PAGE_COUNT              (6U)
#define DIAGNOSTIC_REFRESH_TICKS           (100U)
#define DIAGNOSTIC_PAGE_TICKS              (300U)
#define DIAGNOSTIC_HEARTBEAT_TICKS         (50U)
#define DIAGNOSTIC_LCD_RECOVERY_TICKS      (100U)

volatile SENSOR_DIAGNOSTIC_SNAPSHOT g_sensorDiagnosticSnapshot; /**< 診断結果とLCD状態の共有スナップショット */

static bool s_lcdReady;                                     /**< モジュール内部状態 */
static bool s_autoPage = true;                              /**< モジュール内部状態 */
static uint8_t s_page;                                      /**< モジュール内部状態 */
static uint8_t s_previousSwitchMask;                        /**< モジュール内部状態 */
static uint16_t s_refreshTicks;                             /**< モジュール内部状態 */
static uint16_t s_pageTicks;                                /**< モジュール内部状態 */
static uint16_t s_heartbeatTicks;                           /**< モジュール内部状態 */
static uint16_t s_lcdRecoveryTicks;                         /**< モジュール内部状態 */
static LCD_STATUS s_lastLcdStatus;                          /**< モジュール内部状態 */

/** =================================================================*
 * @brief  SensorDiagnostic_HasSensorError処理
 * @return 実行結果または取得値
 * ================================================================= */
static bool SensorDiagnostic_HasSensorError(void) {
    return (g_sensorDiagnosticSnapshot.sen0206.status != SENSOR_READING_OK) ||
        (g_sensorDiagnosticSnapshot.sen0385.status != SENSOR_READING_OK) ||
        (g_sensorDiagnosticSnapshot.sen0228.status != SENSOR_READING_OK) ||
        (g_sensorDiagnosticSnapshot.sen0193.status != SENSOR_READING_OK);
}

/** =================================================================*
 * @brief  SensorDiagnostic_WriteLine処理
 * @param[in] position 引数
 * @param[in] text 引数
 * @return 実行結果または取得値
 * ================================================================= */
static LCD_STATUS SensorDiagnostic_WriteLine(uint8_t position, const char *text) {
    char line[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
    uint8_t length = 0U;

    memset(line, ' ', LCD_MOST_CHARACTERS_ON_A_LINE);
    line[LCD_MOST_CHARACTERS_ON_A_LINE] = '\0';
    while ((length < LCD_MOST_CHARACTERS_ON_A_LINE) && (text[length] != '\0')) {
        line[length] = text[length];
        ++length;
    }
    return Lcd_Draw(position, line);
}

/** =================================================================*
 * @brief  SensorDiagnostic_FormatTemperature処理
 * @param[out] buffer 引数
 * @param[in] bufferSize 引数
 * @param[in] label 引数
 * @param[in] centiC 引数
 * @return 実行結果または取得値
 * ================================================================= */
static void SensorDiagnostic_FormatTemperature(
    char *buffer,
    uint16_t bufferSize,
    const char *label,
    int16_t centiC) {
    int32_t value = centiC;
    char sign = '+';

    if (value < 0L) {
        sign = '-';
        value = -value;
    }
    (void)snprintf(buffer, bufferSize, "%s:%c%ld.%02ldC",
        label, sign, (long)(value / 100L), (long)(value % 100L));
}

/** =================================================================*
 * @brief  SensorDiagnostic_RenderError処理
 * @param[in] title 引数
 * @param[in] status 引数
 * @return 実行結果または取得値
 * ================================================================= */
static LCD_STATUS SensorDiagnostic_RenderError(
    const char *title,
    SENSOR_READING_STATUS status) {
    char line[17];
    LCD_STATUS lcdStatus;

    (void)snprintf(line, sizeof(line), "ERROR:%s",
        DiagnosticSensors_StatusText(status));
    lcdStatus = SensorDiagnostic_WriteLine(LCD_START_OF_FIRST_LINE, title);
    if (lcdStatus == LCD_STATUS_OK) {
        lcdStatus = SensorDiagnostic_WriteLine(LCD_START_OF_SECOND_LINE, line);
    }
    return lcdStatus;
}

/** =================================================================*
 * @brief  SensorDiagnostic_RenderPage処理
 * @return 実行結果または取得値
 * ================================================================= */
static LCD_STATUS SensorDiagnostic_RenderPage(void) {
    char line1[17];
    char line2[17];
    LCD_STATUS lcdStatus;

    switch (s_page) {
        case 0U:
            (void)snprintf(line1, sizeof(line1), "M:%s S:%s L:%s",
                DiagnosticSensors_StatusText(g_sensorDiagnosticSnapshot.sen0206.status),
                DiagnosticSensors_StatusText(g_sensorDiagnosticSnapshot.sen0385.status),
                DiagnosticSensors_StatusText(g_sensorDiagnosticSnapshot.sen0228.status));
            (void)snprintf(line2, sizeof(line2), "SO:%s TANK:%s",
                DiagnosticSensors_StatusText(g_sensorDiagnosticSnapshot.sen0193.status),
                g_sensorDiagnosticSnapshot.sen0204LiquidDetected ? "ON" : "OFF");
            break;

        case 1U:
            if (g_sensorDiagnosticSnapshot.sen0206.status != SENSOR_READING_OK) {
                return SensorDiagnostic_RenderError("SEN0206 MLX",
                    g_sensorDiagnosticSnapshot.sen0206.status);
            }
            SensorDiagnostic_FormatTemperature(line1, sizeof(line1), "OBJ",
                g_sensorDiagnosticSnapshot.sen0206.objectTemperatureCentiC);
            SensorDiagnostic_FormatTemperature(line2, sizeof(line2), "AMB",
                g_sensorDiagnosticSnapshot.sen0206.ambientTemperatureCentiC);
            break;

        case 2U:
            if (g_sensorDiagnosticSnapshot.sen0385.status != SENSOR_READING_OK) {
                return SensorDiagnostic_RenderError("SEN0385 SHT31",
                    g_sensorDiagnosticSnapshot.sen0385.status);
            }
            SensorDiagnostic_FormatTemperature(line1, sizeof(line1), "AIR",
                g_sensorDiagnosticSnapshot.sen0385.temperatureCentiC);
            (void)snprintf(line2, sizeof(line2), "HUM:%u.%02u%%",
                (unsigned int)(g_sensorDiagnosticSnapshot.sen0385.humidityCentiPercent / 100U),
                (unsigned int)(g_sensorDiagnosticSnapshot.sen0385.humidityCentiPercent % 100U));
            break;

        case 3U:
            if (g_sensorDiagnosticSnapshot.sen0228.status != SENSOR_READING_OK) {
                return SensorDiagnostic_RenderError("SEN0228 VEML",
                    g_sensorDiagnosticSnapshot.sen0228.status);
            }
            (void)snprintf(line1, sizeof(line1), "LUX:%lu",
                (unsigned long)((g_sensorDiagnosticSnapshot.sen0228.illuminanceCentiLux + 50UL) / 100UL));
            (void)snprintf(line2, sizeof(line2), "RAW:%u",
                (unsigned int)g_sensorDiagnosticSnapshot.sen0228.raw);
            break;

        case 4U:
            if (g_sensorDiagnosticSnapshot.sen0193.status != SENSOR_READING_OK) {
                return SensorDiagnostic_RenderError("SEN0193 SOIL",
                    g_sensorDiagnosticSnapshot.sen0193.status);
            }
            (void)snprintf(line1, sizeof(line1), "SOIL RAW:%u",
                (unsigned int)g_sensorDiagnosticSnapshot.sen0193.raw);
            (void)snprintf(line2, sizeof(line2), "INPUT:%umV",
                (unsigned int)g_sensorDiagnosticSnapshot.sen0193.millivolts);
            break;

        case 5U:
        default:
            (void)snprintf(line1, sizeof(line1), "SEN0204 TANK");
            (void)snprintf(line2, sizeof(line2), "LIQUID:%s",
                g_sensorDiagnosticSnapshot.sen0204LiquidDetected ? "YES" : "NO");
            break;
    }

    lcdStatus = SensorDiagnostic_WriteLine(LCD_START_OF_FIRST_LINE, line1);
    if (lcdStatus == LCD_STATUS_OK) {
        lcdStatus = SensorDiagnostic_WriteLine(LCD_START_OF_SECOND_LINE, line2);
    }
    return lcdStatus;
}

/** =================================================================*
 * @brief  SensorDiagnostic_RecordLcdFailure処理
 * @param[in] status 引数
 * @return 実行結果または取得値
 * ================================================================= */
static void SensorDiagnostic_RecordLcdFailure(LCD_STATUS status) {
    s_lcdReady = false;
    s_lastLcdStatus = status;
    ++g_sensorDiagnosticSnapshot.lcdFailureCount;
    Lcd_BacklightOff();
}

/** =================================================================*
 * @brief  SensorDiagnostic_ShowPage処理
 * @return 実行結果または取得値
 * ================================================================= */
static void SensorDiagnostic_ShowPage(void) {
    LCD_STATUS lcdStatus;

    if (!s_lcdReady) {
        return;
    }

    lcdStatus = SensorDiagnostic_RenderPage();
    if (lcdStatus != LCD_STATUS_OK) {
        SensorDiagnostic_RecordLcdFailure(lcdStatus);
    }
}

/** =================================================================*
 * @brief  SensorDiagnostic_TryRecoverLcd処理
 * @return 実行結果または取得値
 * ================================================================= */
static void SensorDiagnostic_TryRecoverLcd(void) {
    LCD_STATUS lcdStatus;

    if (s_lcdReady) {
        return;
    }

    ++g_sensorDiagnosticSnapshot.lcdRecoveryAttemptCount;
    Lcd_PeripheralInit();
    lcdStatus = Lcd_Init();
    if (lcdStatus == LCD_STATUS_OK) {
        lcdStatus = Lcd_DisplayOnOff(
            LCD_DISPLAY_ON, LCD_CURSOR_OFF, LCD_CURSOR_BLINK_OFF);
    }
    if (lcdStatus == LCD_STATUS_OK) {
        Lcd_BacklightOn();
        lcdStatus = SensorDiagnostic_RenderPage();
    }
    if (lcdStatus == LCD_STATUS_OK) {
        s_lcdReady = true;
        ++g_sensorDiagnosticSnapshot.lcdRecoverySuccessCount;
    } else {
        SensorDiagnostic_RecordLcdFailure(lcdStatus);
    }
}

/** =================================================================*
 * @brief  SensorDiagnostic_UpdateLcdTelemetry処理
 * @return 実行結果または取得値
 * ================================================================= */
static void SensorDiagnostic_UpdateLcdTelemetry(void) {
    g_sensorDiagnosticSnapshot.lcdReady = s_lcdReady;
    g_sensorDiagnosticSnapshot.currentPage = s_page;
    g_sensorDiagnosticSnapshot.lastLcdStatus = (uint8_t)s_lastLcdStatus;
    LedControl_Set(LED_CONTROL_3, !s_lcdReady);
}

/** =================================================================*
 * @brief  SensorDiagnostic_Refresh処理
 * @return 実行結果または取得値
 * ================================================================= */
static void SensorDiagnostic_Refresh(void) {
    SEN0206_READING sen0206 = {SENSOR_READING_INVALID_DATA, 0, 0};
    SEN0385_READING sen0385 = {SENSOR_READING_INVALID_DATA, 0, 0U};
    SEN0228_READING sen0228 = {SENSOR_READING_INVALID_DATA, 0UL, 0U};
    SEN0193_READING sen0193 = {SENSOR_READING_INVALID_DATA, 0U, 0U};

    DiagnosticSensors_ReadSen0206(&sen0206);
    DiagnosticSensors_ReadSen0385(&sen0385);
    DiagnosticSensors_ReadSen0228(&sen0228);
    DiagnosticSensors_ReadSen0193(&sen0193);
    g_sensorDiagnosticSnapshot.sen0206 = sen0206;
    g_sensorDiagnosticSnapshot.sen0385 = sen0385;
    g_sensorDiagnosticSnapshot.sen0228 = sen0228;
    g_sensorDiagnosticSnapshot.sen0193 = sen0193;
    g_sensorDiagnosticSnapshot.sen0204LiquidDetected = DiagnosticSensors_ReadSen0204();
    ++g_sensorDiagnosticSnapshot.refreshCount;

    LedControl_Set(LED_CONTROL_2, SensorDiagnostic_HasSensorError());
    SensorDiagnostic_ShowPage();
}

/** =================================================================*
 * @brief  SensorDiagnostic_Init処理
 * ================================================================= */
void SensorDiagnostic_Init(void) {
    LCD_STATUS lcdStatus;

    g_sensorDiagnosticSnapshot.sen0206.status = SENSOR_READING_INVALID_DATA;
    g_sensorDiagnosticSnapshot.sen0385.status = SENSOR_READING_INVALID_DATA;
    g_sensorDiagnosticSnapshot.sen0228.status = SENSOR_READING_INVALID_DATA;
    g_sensorDiagnosticSnapshot.sen0193.status = SENSOR_READING_INVALID_DATA;
    g_sensorDiagnosticSnapshot.sen0204LiquidDetected = false;
    g_sensorDiagnosticSnapshot.lcdReady = false;
    g_sensorDiagnosticSnapshot.currentPage = 0U;
    g_sensorDiagnosticSnapshot.lastLcdStatus = (uint8_t)LCD_STATUS_OK;
    g_sensorDiagnosticSnapshot.lcdFailureCount = 0UL;
    g_sensorDiagnosticSnapshot.lcdRecoveryAttemptCount = 0UL;
    g_sensorDiagnosticSnapshot.lcdRecoverySuccessCount = 0UL;
    g_sensorDiagnosticSnapshot.refreshCount = 0UL;
    s_lastLcdStatus = LCD_STATUS_OK;

    (void)Board_Init();
    TimeControlInit();
    Lcd_PeripheralInit();
    lcdStatus = Lcd_Init();
    if (lcdStatus == LCD_STATUS_OK) {
        lcdStatus = Lcd_DisplayOnOff(
            LCD_DISPLAY_ON, LCD_CURSOR_OFF, LCD_CURSOR_BLINK_OFF);
    }
    s_lcdReady = (lcdStatus == LCD_STATUS_OK);
    if (s_lcdReady) {
        Lcd_BacklightOn();
        lcdStatus = SensorDiagnostic_WriteLine(LCD_START_OF_FIRST_LINE, "SENSOR TEST");
        if (lcdStatus == LCD_STATUS_OK) {
            lcdStatus = SensorDiagnostic_WriteLine(LCD_START_OF_SECOND_LINE, "READING...");
        }
        if (lcdStatus != LCD_STATUS_OK) {
            SensorDiagnostic_RecordLcdFailure(lcdStatus);
        }
    } else {
        SensorDiagnostic_RecordLcdFailure(lcdStatus);
    }

    DiagnosticSensors_Init();
    s_page = 0U;
    s_autoPage = true;
    s_previousSwitchMask = 0U;
    s_refreshTicks = 0U;
    s_pageTicks = 0U;
    s_heartbeatTicks = 0U;
    s_lcdRecoveryTicks = 0U;
    SensorDiagnostic_Refresh();
    SensorDiagnostic_UpdateLcdTelemetry();
}

/** =================================================================*
 * @brief  SensorDiagnostic_RunOnce処理
 * ================================================================= */
void SensorDiagnostic_RunOnce(void) {
    uint8_t pressed;
    uint8_t pressedEdge;

    Board_ServiceWatchdog();
    if (!TimeControlDelayMs(10U)) {
        return;
    }
    (void)Board_Process10Ms();

    pressed = Board_GetPressedSwitchMask();
    pressedEdge = (uint8_t)(pressed & (uint8_t)~s_previousSwitchMask);
    s_previousSwitchMask = pressed;

    if ((pressedEdge & SWITCH_CONTROL_PSW1) != 0U) {
        s_page = (s_page == 0U) ? (DIAGNOSTIC_PAGE_COUNT - 1U) : (uint8_t)(s_page - 1U);
        s_pageTicks = 0U;
        SensorDiagnostic_ShowPage();
    }
    if ((pressedEdge & SWITCH_CONTROL_PSW2) != 0U) {
        s_page = (uint8_t)((s_page + 1U) % DIAGNOSTIC_PAGE_COUNT);
        s_pageTicks = 0U;
        SensorDiagnostic_ShowPage();
    }
    if ((pressedEdge & SWITCH_CONTROL_PSW3) != 0U) {
        s_autoPage = !s_autoPage;
    }
    if ((pressedEdge & SWITCH_CONTROL_PSW4) != 0U) {
        s_refreshTicks = 0U;
        SensorDiagnostic_Refresh();
    }

    if (++s_refreshTicks >= DIAGNOSTIC_REFRESH_TICKS) {
        s_refreshTicks = 0U;
        SensorDiagnostic_Refresh();
    }
    if (s_autoPage && (++s_pageTicks >= DIAGNOSTIC_PAGE_TICKS)) {
        s_pageTicks = 0U;
        s_page = (uint8_t)((s_page + 1U) % DIAGNOSTIC_PAGE_COUNT);
        SensorDiagnostic_ShowPage();
    }
    if (!s_lcdReady && (++s_lcdRecoveryTicks >= DIAGNOSTIC_LCD_RECOVERY_TICKS)) {
        s_lcdRecoveryTicks = 0U;
        SensorDiagnostic_TryRecoverLcd();
    } else if (s_lcdReady) {
        s_lcdRecoveryTicks = 0U;
    }
    if (++s_heartbeatTicks >= DIAGNOSTIC_HEARTBEAT_TICKS) {
        s_heartbeatTicks = 0U;
        LedControl_Toggle(LED_CONTROL_1);
    }

    SensorDiagnostic_UpdateLcdTelemetry();
}
