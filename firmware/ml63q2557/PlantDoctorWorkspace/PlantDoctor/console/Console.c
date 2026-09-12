/** =================================================================*
 * @file   Console.c
 * @brief  UARTコンソールコマンド解釈API実装 (P2-4)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "Console.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "PlantDoctorConfig.h"

#define CONSOLE_RX_LINE_MAX         (PLANT_DOCTOR_CONSOLE_LINE_MAX)
#define CONSOLE_TX_BUF_SIZE         (PLANT_DOCTOR_CONSOLE_TX_BUFFER_SIZE)

static CONSOLE_SERVICES s_services;
static char s_rxLine[CONSOLE_RX_LINE_MAX + 1U];
static uint8_t s_rxLen;
static bool s_rxOverflow;

static char s_txBuf[CONSOLE_TX_BUF_SIZE];
static uint16_t s_txHead;
static uint16_t s_txTail;
static uint16_t s_txCount;

static bool s_dumpActive;
static uint16_t s_dumpIndex;
static uint16_t s_dumpTotal;

static const char *s_statusNames[] = {
    "HEALTHY",
    "HEAT_STRESS",
    "DRY_STRESS",
    "LOW_LIGHT",
    "ROOT_UPTAKE",
    "SENSOR_FAULT",
    "DEGRADED_SOIL"
};

/** =================================================================*
 * @brief  うるう年判定処理
 * @param[in] year 西暦年
 * @return 判定結果
 * ================================================================= */
static bool Console_IsLeapYear(uint16_t year) {
    if ((year % 400U) == 0U) {
        return true;
    }
    if ((year % 100U) == 0U) {
        return false;
    }
    return ((year % 4U) == 0U);
}

/** =================================================================*
 * @brief  月の日数取得処理
 * @param[in] year 西暦年
 * @param[in] month 月 (1〜12)
 * @return 日数 (1〜31)
 * ================================================================= */
static uint8_t Console_GetDaysInMonth(uint16_t year, uint8_t month) {
    static const uint8_t days[12] = {
        31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((month < 1U) || (month > 12U)) {
        return 0U;
    }
    if ((month == 2U) && Console_IsLeapYear(year)) {
        return 29U;
    }
    return days[month - 1U];
}

/** =================================================================*
 * @brief  日時からUNIX秒への変換処理
 * ================================================================= */
static uint32_t Console_DateToUnix(uint16_t year, uint8_t month, uint8_t day,
                                   uint8_t hour, uint8_t minute, uint8_t second) {
    uint32_t days = 0UL;
    uint16_t y;
    uint8_t m;

    for (y = 1970U; y < year; ++y) {
        days += Console_IsLeapYear(y) ? 366UL : 365UL;
    }
    for (m = 1U; m < month; ++m) {
        days += (uint32_t)Console_GetDaysInMonth(year, m);
    }
    days += (uint32_t)(day - 1U);

    return (days * 86400UL) +
           ((uint32_t)hour * 3600UL) +
           ((uint32_t)minute * 60UL) +
           (uint32_t)second;
}

/** =================================================================*
 * @brief  UNIX秒から日時への変換処理
 * ================================================================= */
static void Console_UnixToDate(uint32_t unixSec, uint16_t *year, uint8_t *month,
                               uint8_t *day, uint8_t *hour, uint8_t *minute, uint8_t *second) {
    uint32_t days = unixSec / 86400UL;
    uint32_t rem = unixSec % 86400UL;
    uint16_t y = 1970U;
    uint8_t m = 1U;
    uint32_t yearDays;

    *hour = (uint8_t)(rem / 3600UL);
    rem %= 3600UL;
    *minute = (uint8_t)(rem / 60UL);
    *second = (uint8_t)(rem % 60UL);

    while (true) {
        yearDays = Console_IsLeapYear(y) ? 366UL : 365UL;
        if (days < yearDays) {
            break;
        }
        days -= yearDays;
        ++y;
    }
    *year = y;

    while (true) {
        uint8_t dim = Console_GetDaysInMonth(y, m);
        if (days < (uint32_t)dim) {
            break;
        }
        days -= (uint32_t)dim;
        ++m;
    }
    *month = m;
    *day = (uint8_t)(days + 1UL);
}

/** =================================================================*
 * @brief  送信リングバッファへ文字列をエンキューする。
 * ================================================================= */
static void Console_EnqueueTxString(const char *str) {
    if (str == NULL) {
        return;
    }
    while (*str != '\0') {
        if (s_txCount < CONSOLE_TX_BUF_SIZE) {
            s_txBuf[s_txHead] = *str;
            s_txHead = (uint16_t)((s_txHead + 1U) % CONSOLE_TX_BUF_SIZE);
            ++s_txCount;
        }
        ++str;
    }
}

/** =================================================================*
 * @brief  Console_Init処理
 * ================================================================= */
void Console_Init(const CONSOLE_SERVICES *services) {
    if (services != NULL) {
        s_services = *services;
    } else {
        (void)memset(&s_services, 0, sizeof(s_services));
    }
    Console_Reset();
}

/** =================================================================*
 * @brief  Console_Reset処理
 * ================================================================= */
void Console_Reset(void) {
    s_rxLen = 0U;
    s_rxOverflow = false;
    s_txHead = 0U;
    s_txTail = 0U;
    s_txCount = 0U;
    s_dumpActive = false;
    s_dumpIndex = 0U;
    s_dumpTotal = 0U;
}

/** =================================================================*
 * @brief  Console_IsDumping処理
 * ================================================================= */
bool Console_IsDumping(void) {
    return s_dumpActive;
}

/** =================================================================*
 * @brief  Console_HasTxData処理
 * ================================================================= */
bool Console_HasTxData(void) {
    return (s_txCount > 0U);
}

/** =================================================================*
 * @brief  Console_GetTxChar処理
 * ================================================================= */
bool Console_GetTxChar(char *ch) {
    if ((ch == NULL) || (s_txCount == 0U)) {
        return false;
    }
    *ch = s_txBuf[s_txTail];
    s_txTail = (uint16_t)((s_txTail + 1U) % CONSOLE_TX_BUF_SIZE);
    --s_txCount;
    return true;
}

/** =================================================================*
 * @brief  Console_ExecuteCommand処理 (同期実行)
 * ================================================================= */
bool Console_ExecuteCommand(const char *cmd, char *response, uint16_t maxLen) {
    if ((cmd == NULL) || (response == NULL) || (maxLen == 0U)) {
        return false;
    }
    response[0] = '\0';

    if (strcmp(cmd, "V") == 0) {
        uint8_t tick = s_services.getMaxPendingTicks ? s_services.getMaxPendingTicks() : 0U;
        (void)snprintf(response, maxLen, "OK build=PlantDoctor tick=%u\r\n", (unsigned int)tick);
        return true;
    }

    if (strcmp(cmd, "S") == 0) {
        PLANT_SENSOR_SNAPSHOT snap;
        if ((s_services.getSnapshot == NULL) || !s_services.getSnapshot(&snap)) {
            (void)snprintf(response, maxLen, "ERR no_sensor_data\r\n");
            return false;
        }
        (void)snprintf(response, maxLen,
                       "OK seq=%u soil=%u leaf=%d air=%d hum=%u lux=%u tank=%u\r\n",
                       (unsigned int)snap.sampleSequence,
                       (unsigned int)snap.soilMoistureRaw,
                       (int)snap.leafTemperatureCentiC,
                       (int)snap.airTemperatureCentiC,
                       (unsigned int)snap.relativeHumidityCentiPercent,
                       (unsigned int)snap.illuminanceRaw,
                       snap.tankLiquidDetected ? 1U : 0U);
        return true;
    }

    if (strcmp(cmd, "Q") == 0) {
        uint8_t stress = 0U;
        uint8_t status = 0U;
        const char *soil = NULL;
        const char *statusStr = "UNKNOWN";

        if ((s_services.getDiagnosis == NULL) || !s_services.getDiagnosis(&stress, &status, &soil)) {
            (void)snprintf(response, maxLen, "ERR no_diagnosis\r\n");
            return false;
        }
        if (status < (sizeof(s_statusNames) / sizeof(s_statusNames[0]))) {
            statusStr = s_statusNames[status];
        }
        if (soil == NULL) {
            soil = "UNKNOWN";
        }
        (void)snprintf(response, maxLen, "OK stress=%u status=%s soil=%s\r\n",
                       (unsigned int)stress, statusStr, soil);
        return true;
    }

    if (strcmp(cmd, "T?") == 0) {
        uint32_t sec;
        uint16_t year;
        uint8_t month, day, hour, minute, second;

        if (s_services.getUnixSeconds == NULL) {
            (void)snprintf(response, maxLen, "ERR not_supported\r\n");
            return false;
        }
        sec = s_services.getUnixSeconds();
        if (sec == 0xFFFFFFFFUL) {
            (void)snprintf(response, maxLen, "OK NOT_SET\r\n");
            return true;
        }
        Console_UnixToDate(sec, &year, &month, &day, &hour, &minute, &second);
        (void)snprintf(response, maxLen, "OK %04u-%02u-%02u %02u:%02u:%02u\r\n",
                       (unsigned int)year, (unsigned int)month, (unsigned int)day,
                       (unsigned int)hour, (unsigned int)minute, (unsigned int)second);
        return true;
    }

    if (strncmp(cmd, "T=", 2) == 0) {
        const char *dt = &cmd[2];
        uint16_t year;
        uint8_t month, day, hour, minute, second;
        uint32_t unixSec;
        char numBuf[5];
        uint8_t i;

        if (strlen(dt) != 14U) {
            (void)snprintf(response, maxLen, "ERR invalid_datetime\r\n");
            return false;
        }
        for (i = 0U; i < 14U; ++i) {
            if ((dt[i] < '0') || (dt[i] > '9')) {
                (void)snprintf(response, maxLen, "ERR invalid_datetime\r\n");
                return false;
            }
        }

        numBuf[4] = '\0';
        (void)memcpy(numBuf, dt, 4U);
        year = (uint16_t)atoi(numBuf);

        numBuf[2] = '\0';
        (void)memcpy(numBuf, &dt[4], 2U);
        month = (uint8_t)atoi(numBuf);

        (void)memcpy(numBuf, &dt[6], 2U);
        day = (uint8_t)atoi(numBuf);

        (void)memcpy(numBuf, &dt[8], 2U);
        hour = (uint8_t)atoi(numBuf);

        (void)memcpy(numBuf, &dt[10], 2U);
        minute = (uint8_t)atoi(numBuf);

        (void)memcpy(numBuf, &dt[12], 2U);
        second = (uint8_t)atoi(numBuf);

        if ((year < 2024U) || (month < 1U) || (month > 12U) ||
            (day < 1U) || (day > Console_GetDaysInMonth(year, month)) ||
            (hour > 23U) || (minute > 59U) || (second > 59U)) {
            (void)snprintf(response, maxLen, "ERR invalid_datetime\r\n");
            return false;
        }

        unixSec = Console_DateToUnix(year, month, day, hour, minute, second);
        if ((s_services.setUnixSeconds == NULL) || !s_services.setUnixSeconds(unixSec)) {
            (void)snprintf(response, maxLen, "ERR datetime_set_failed\r\n");
            return false;
        }
        (void)snprintf(response, maxLen, "OK\r\n");
        return true;
    }

    if (strcmp(cmd, "C?") == 0) {
        uint16_t dry = 0U, wet = 0U;
        if ((s_services.getCalibration == NULL) || !s_services.getCalibration(&dry, &wet)) {
            (void)snprintf(response, maxLen, "ERR calib_unavailable\r\n");
            return false;
        }
        (void)snprintf(response, maxLen, "OK dry=%u wet=%u\r\n",
                       (unsigned int)dry, (unsigned int)wet);
        return true;
    }

    if (strncmp(cmd, "C=", 2) == 0) {
        const char *comma = strchr(&cmd[2], ',');
        int dryVal, wetVal;

        if (comma == NULL) {
            (void)snprintf(response, maxLen, "ERR invalid_calib\r\n");
            return false;
        }
        dryVal = atoi(&cmd[2]);
        wetVal = atoi(comma + 1);

        if ((dryVal < 0) || (dryVal > 4095) || (wetVal < 0) || (wetVal > 4095) ||
            (dryVal >= wetVal) || ((wetVal - dryVal) < (int)PLANT_DOCTOR_SOIL_CAL_MIN_DIFF)) {
            (void)snprintf(response, maxLen, "ERR invalid_calib\r\n");
            return false;
        }

        if ((s_services.setCalibration == NULL) ||
            !s_services.setCalibration((uint16_t)dryVal, (uint16_t)wetVal)) {
            (void)snprintf(response, maxLen, "ERR calib_set_failed\r\n");
            return false;
        }
        (void)snprintf(response, maxLen, "OK\r\n");
        return true;
    }

    if (strcmp(cmd, "D") == 0) {
        uint16_t count = s_services.getRecordCount ? s_services.getRecordCount() : 0U;
        if (count == 0U) {
            (void)snprintf(response, maxLen, "OK records=0\r\n");
            return true;
        }
        s_dumpActive = true;
        s_dumpIndex = 0U;
        s_dumpTotal = count;
        (void)snprintf(response, maxLen, "OK dump_started\r\n");
        return true;
    }

    if (strcmp(cmd, "X!") == 0) {
        if ((s_services.eraseLog == NULL) || !s_services.eraseLog()) {
            (void)snprintf(response, maxLen, "ERR erase_failed\r\n");
            return false;
        }
        (void)snprintf(response, maxLen, "OK\r\n");
        return true;
    }

    if (strcmp(cmd, "X") == 0) {
        (void)snprintf(response, maxLen, "ERR confirm_required\r\n");
        return false;
    }

    if (strncmp(cmd, "M=", 2) == 0) {
        if ((cmd[2] == '0') && (cmd[3] == '\0')) {
            if ((s_services.setDemoMode == NULL) || !s_services.setDemoMode(false)) {
                (void)snprintf(response, maxLen, "ERR mode_set_failed\r\n");
                return false;
            }
            (void)snprintf(response, maxLen, "OK\r\n");
            return true;
        }
        if ((cmd[2] == '1') && (cmd[3] == '\0')) {
            if ((s_services.setDemoMode == NULL) || !s_services.setDemoMode(true)) {
                (void)snprintf(response, maxLen, "ERR mode_set_failed\r\n");
                return false;
            }
            (void)snprintf(response, maxLen, "OK\r\n");
            return true;
        }
        (void)snprintf(response, maxLen, "ERR invalid_mode\r\n");
        return false;
    }

    (void)snprintf(response, maxLen, "ERR unknown_cmd\r\n");
    return false;
}

/** =================================================================*
 * @brief  Console_PutRxChar処理
 * ================================================================= */
void Console_PutRxChar(char ch) {
    if ((ch == '\r') || (ch == '\n')) {
        if (s_rxOverflow) {
            Console_EnqueueTxString("ERR line_too_long\r\n");
            s_rxLen = 0U;
            s_rxOverflow = false;
        } else if (s_rxLen > 0U) {
            char resp[128];
            s_rxLine[s_rxLen] = '\0';
            (void)Console_ExecuteCommand(s_rxLine, resp, sizeof(resp));
            Console_EnqueueTxString(resp);
            s_rxLen = 0U;
        }
        return;
    }

    if (s_rxOverflow) {
        return;
    }

    if (s_rxLen < CONSOLE_RX_LINE_MAX) {
        s_rxLine[s_rxLen] = ch;
        ++s_rxLen;
    } else {
        s_rxOverflow = true;
    }
}

/** =================================================================*
 * @brief  Console_Process10Ms処理
 * ================================================================= */
void Console_Process10Ms(void) {
    /* ダンプ状態機械: 1 tickあたり1レコードを処理 */
    if (s_dumpActive) {
        /* 送信バッファにCSV1行分(約80B)の空きがあるか確認 */
        if ((CONSOLE_TX_BUF_SIZE - s_txCount) >= 80U) {
            PLANT_LOG_RECORD record;
            if ((s_services.readRecord != NULL) && s_services.readRecord(s_dumpIndex, &record)) {
                char csvLine[80];
                (void)PlantLogRecord_FormatCsv(&record, csvLine, sizeof(csvLine));
                Console_EnqueueTxString(csvLine);
                Console_EnqueueTxString("\r\n");
            }
            ++s_dumpIndex;
            if (s_dumpIndex >= s_dumpTotal) {
                Console_EnqueueTxString("OK\r\n");
                s_dumpActive = false;
            }
        }
    }
}
