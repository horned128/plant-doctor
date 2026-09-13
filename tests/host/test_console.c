/** =================================================================*
 * @file   test_console.c
 * @brief  UARTコンソール単体テスト (P2-4)
 * ================================================================= */
#include "assert_helper.h"
#include "../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/console/Console.h"
#include "../../firmware/ml63q2557/PlantDoctorWorkspace/PlantDoctor/storage/PlantLogRecord.h"
#include <string.h>
#include <stdio.h>

static uint8_t s_mockPendingTicks = 0U;
static bool s_mockSnapshotValid = true;
static PLANT_SENSOR_SNAPSHOT s_mockSnapshot;
static bool s_mockDiagnosisValid = true;
static uint8_t s_mockStress = 0U;
static uint8_t s_mockStatus = 0U;
static const char *s_mockSoilTrend = "STABLE";

static uint32_t s_mockUnixSec = 0xFFFFFFFFUL;
static uint32_t s_lastSetUnixSec = 0UL;
static bool s_setUnixSecSuccess = true;

static uint16_t s_mockCalDry = 1000U;
static uint16_t s_mockCalWet = 3000U;
static uint16_t s_lastSetDry = 0U;
static uint16_t s_lastSetWet = 0U;

static bool s_eraseLogCalled = false;
static bool s_eraseLogSuccess = true;

static bool s_lastDemoMode = false;
static bool s_demoModeSuccess = true;

static uint16_t s_mockRecordCount = 0U;
static PLANT_LOG_RECORD s_mockRecords[5];

static uint8_t Mock_GetMaxPendingTicks(void) {
    return s_mockPendingTicks;
}

static bool Mock_GetSnapshot(PLANT_SENSOR_SNAPSHOT *snap) {
    if (!s_mockSnapshotValid || (snap == NULL)) {
        return false;
    }
    *snap = s_mockSnapshot;
    return true;
}

static bool Mock_GetDiagnosis(uint8_t *stress, uint8_t *status, const char **soil) {
    if (!s_mockDiagnosisValid) {
        return false;
    }
    if (stress) *stress = s_mockStress;
    if (status) *status = s_mockStatus;
    if (soil) *soil = s_mockSoilTrend;
    return true;
}

static uint32_t Mock_GetUnixSeconds(void) {
    return s_mockUnixSec;
}

static bool Mock_SetUnixSeconds(uint32_t sec) {
    s_lastSetUnixSec = sec;
    return s_setUnixSecSuccess;
}

static bool Mock_GetCalibration(uint16_t *dry, uint16_t *wet) {
    if (dry) *dry = s_mockCalDry;
    if (wet) *wet = s_mockCalWet;
    return true;
}

static bool Mock_SetCalibration(uint16_t dry, uint16_t wet) {
    s_lastSetDry = dry;
    s_lastSetWet = wet;
    return true;
}

static bool Mock_EraseLog(void) {
    s_eraseLogCalled = true;
    return s_eraseLogSuccess;
}

static bool Mock_SetDemoMode(bool enable) {
    s_lastDemoMode = enable;
    return s_demoModeSuccess;
}

static uint16_t Mock_GetRecordCount(void) {
    return s_mockRecordCount;
}

static bool Mock_ReadRecord(uint16_t index, PLANT_LOG_RECORD *record) {
    if ((index >= s_mockRecordCount) || (record == NULL)) {
        return false;
    }
    *record = s_mockRecords[index];
    return true;
}

static uint8_t s_mockWateringStatus = 0U;
static bool s_mockPumpOn = false;

static uint8_t Mock_TriggerWatering(void) {
    return s_mockWateringStatus;
}

static bool Mock_IsPumpOn(void) {
    return s_mockPumpOn;
}

static CONSOLE_SERVICES s_testServices = {
    .getRecordCount = Mock_GetRecordCount,
    .readRecord = Mock_ReadRecord,
    .eraseLog = Mock_EraseLog,
    .getUnixSeconds = Mock_GetUnixSeconds,
    .setUnixSeconds = Mock_SetUnixSeconds,
    .getSnapshot = Mock_GetSnapshot,
    .getDiagnosis = Mock_GetDiagnosis,
    .getCalibration = Mock_GetCalibration,
    .setCalibration = Mock_SetCalibration,
    .getMaxPendingTicks = Mock_GetMaxPendingTicks,
    .setDemoMode = Mock_SetDemoMode,
    .triggerWatering = Mock_TriggerWatering,
    .isPumpOn = Mock_IsPumpOn
};

/* ================================================================= */

static void test_version_command(void) {
    char resp[128];
    bool ok;

    s_mockPendingTicks = 2U;
    Console_Init(&s_testServices);

    ok = Console_ExecuteCommand("V", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK build=PlantDoctor tick=2") != NULL);
}

static void test_sensor_command(void) {
    char resp[128];
    bool ok;

    Console_Init(&s_testServices);
    s_mockSnapshotValid = true;
    s_mockSnapshot.sampleSequence = 123U;
    s_mockSnapshot.soilMoistureRaw = 2048U;
    s_mockSnapshot.leafTemperatureCentiC = 2550;
    s_mockSnapshot.airTemperatureCentiC = 2450;
    s_mockSnapshot.relativeHumidityCentiPercent = 6000U;
    s_mockSnapshot.illuminanceRaw = 1500U;
    s_mockSnapshot.tankLiquidDetected = true;

    ok = Console_ExecuteCommand("S", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK seq=123 soil=2048 leaf=2550 air=2450 hum=6000 lux=1500 tank=1") != NULL);

    s_mockSnapshotValid = false;
    ok = Console_ExecuteCommand("S", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR no_sensor_data") != NULL);
}

static void test_diagnosis_command(void) {
    char resp[128];
    bool ok;

    Console_Init(&s_testServices);
    s_mockDiagnosisValid = true;
    s_mockStress = 50U;
    s_mockStatus = 1U; /* HEAT_STRESS */
    s_mockSoilTrend = "DRYING";

    ok = Console_ExecuteCommand("Q", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK stress=50 status=HEAT_STRESS soil=DRYING") != NULL);
}

static void test_time_commands(void) {
    char resp[128];
    bool ok;

    Console_Init(&s_testServices);

    /* T? when not set */
    s_mockUnixSec = 0xFFFFFFFFUL;
    ok = Console_ExecuteCommand("T?", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK NOT_SET") != NULL);

    /* T? when set to 2026-03-25 14:30:00 */
    s_mockUnixSec = 1774449000UL; /* 2026-03-25 14:30:00 */
    ok = Console_ExecuteCommand("T?", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK 2026-03-25 14:30:00") != NULL);

    /* T= set valid */
    s_setUnixSecSuccess = true;
    ok = Console_ExecuteCommand("T=20260325143000", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT(1774449000UL, s_lastSetUnixSec);

    /* T= invalid non-leap day (2026-02-29) */
    ok = Console_ExecuteCommand("T=20260229120000", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR invalid_datetime") != NULL);

    /* T= invalid month 13 */
    ok = Console_ExecuteCommand("T=20261301000000", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR invalid_datetime") != NULL);

    /* T= length error */
    ok = Console_ExecuteCommand("T=2026", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR invalid_datetime") != NULL);
}

static void test_calibration_commands(void) {
    char resp[128];
    bool ok;

    Console_Init(&s_testServices);

    s_mockCalDry = 1000U;
    s_mockCalWet = 3000U;
    ok = Console_ExecuteCommand("C?", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK dry=1000 wet=3000") != NULL);

    /* C= valid */
    ok = Console_ExecuteCommand("C=1200,3400", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT(1200U, s_lastSetDry);
    TEST_ASSERT_EQUAL_UINT(3400U, s_lastSetWet);

    /* C= dry >= wet */
    ok = Console_ExecuteCommand("C=3000,1000", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR invalid_calib") != NULL);

    /* C= diff < 200 */
    ok = Console_ExecuteCommand("C=1000,1100", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR invalid_calib") != NULL);

    /* C= format error */
    ok = Console_ExecuteCommand("C=bad", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR invalid_calib") != NULL);
}

static void test_erase_commands(void) {
    char resp[128];
    bool ok;

    Console_Init(&s_testServices);

    /* X alone without ! requires confirm */
    s_eraseLogCalled = false;
    ok = Console_ExecuteCommand("X", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_FALSE(s_eraseLogCalled);
    TEST_ASSERT_TRUE(strstr(resp, "ERR confirm_required") != NULL);

    /* X! erases */
    s_eraseLogSuccess = true;
    ok = Console_ExecuteCommand("X!", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(s_eraseLogCalled);
    TEST_ASSERT_TRUE(strstr(resp, "OK") != NULL);
}

static void test_demo_mode_commands(void) {
    char resp[128];
    bool ok;

    Console_Init(&s_testServices);

    ok = Console_ExecuteCommand("M=1", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(s_lastDemoMode);

    ok = Console_ExecuteCommand("M=0", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_FALSE(s_lastDemoMode);

    ok = Console_ExecuteCommand("M=5", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR invalid_mode") != NULL);
}

static void test_streaming_dump(void) {
    char line[128];
    uint16_t len;
    char ch;

    Console_Init(&s_testServices);

    /* Setup 2 mock records */
    s_mockRecordCount = 2U;
    memset(&s_mockRecords[0], 0, sizeof(s_mockRecords[0]));
    s_mockRecords[0].timestampSeconds = 1774449000UL;
    s_mockRecords[0].recordSequence = 1U;
    s_mockRecords[0].validFlags = 0x3FU;
    s_mockRecords[0].soilMoistureRaw = 2000U;

    memset(&s_mockRecords[1], 0, sizeof(s_mockRecords[1]));
    s_mockRecords[1].timestampSeconds = 1774449060UL;
    s_mockRecords[1].recordSequence = 2U;
    s_mockRecords[1].validFlags = 0x3FU;
    s_mockRecords[1].soilMoistureRaw = 1950U;

    /* Feed D\r */
    Console_PutRxChar('D');
    Console_PutRxChar('\r');

    TEST_ASSERT_TRUE(Console_IsDumping());

    /* Tick 1: outputs record 0 */
    Console_Process10Ms();
    TEST_ASSERT_TRUE(Console_HasTxData());

    len = 0U;
    while (Console_GetTxChar(&ch)) {
        line[len++] = ch;
    }
    line[len] = '\0';
    TEST_ASSERT_TRUE(strstr(line, "1774449000") != NULL);

    /* Tick 2: outputs record 1 and then OK */
    Console_Process10Ms();
    len = 0U;
    while (Console_GetTxChar(&ch)) {
        line[len++] = ch;
    }
    line[len] = '\0';
    TEST_ASSERT_TRUE(strstr(line, "1774449060") != NULL);
    TEST_ASSERT_TRUE(strstr(line, "OK") != NULL);
    TEST_ASSERT_FALSE(Console_IsDumping());
}

static void test_streaming_overflow(void) {
    char line[128];
    uint16_t len;
    char ch;
    uint8_t i;

    Console_Init(&s_testServices);

    /* Feed 35 'A' chars then '\n' -> overflow! */
    for (i = 0U; i < 35U; ++i) {
        Console_PutRxChar('A');
    }
    Console_PutRxChar('\n');

    TEST_ASSERT_TRUE(Console_HasTxData());
    len = 0U;
    while (Console_GetTxChar(&ch)) {
        line[len++] = ch;
    }
    line[len] = '\0';
    TEST_ASSERT_TRUE(strstr(line, "ERR line_too_long") != NULL);

    /* Next command should process normally */
    Console_PutRxChar('V');
    Console_PutRxChar('\r');
    len = 0U;
    while (Console_GetTxChar(&ch)) {
        line[len++] = ch;
    }
    line[len] = '\0';
    TEST_ASSERT_TRUE(strstr(line, "OK build=PlantDoctor") != NULL);
}

static void test_watering_commands(void) {
    char resp[128];
    bool ok;

    Console_Init(&s_testServices);

    /* Test W? when pump is off */
    s_mockPumpOn = false;
    ok = Console_ExecuteCommand("W?", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK pump=0") != NULL);

    /* Test W? when pump is on */
    s_mockPumpOn = true;
    ok = Console_ExecuteCommand("W?", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK pump=1") != NULL);

    /* Test W with OK */
    s_mockWateringStatus = 0U;
    ok = Console_ExecuteCommand("W", resp, sizeof(resp));
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "OK watering_started") != NULL);

    /* Test W with tank empty */
    s_mockWateringStatus = 1U;
    ok = Console_ExecuteCommand("W", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR tank_empty") != NULL);

    /* Test W with cooldown */
    s_mockWateringStatus = 2U;
    ok = Console_ExecuteCommand("W", resp, sizeof(resp));
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(strstr(resp, "ERR cooldown") != NULL);
}

int main(void) {
    TEST_RUN(test_version_command);
    TEST_RUN(test_sensor_command);
    TEST_RUN(test_diagnosis_command);
    TEST_RUN(test_time_commands);
    TEST_RUN(test_calibration_commands);
    TEST_RUN(test_erase_commands);
    TEST_RUN(test_demo_mode_commands);
    TEST_RUN(test_watering_commands);
    TEST_RUN(test_streaming_dump);
    TEST_RUN(test_streaming_overflow);

    TEST_REPORT_AND_EXIT();
}
