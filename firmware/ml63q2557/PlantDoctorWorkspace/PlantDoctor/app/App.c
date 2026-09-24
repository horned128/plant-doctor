/** =================================================================*
 * @file   App.c
 * @brief  アプリケーション処理
 * ================================================================= */
#include "App.h"                                            /* AppのAPIと型定義 */
#include "AppStateMachine.h"                                /* AppStateMachineのAPIと型定義 */
#include "Board.h"                                          /* BoardのAPIと型定義 */
#include "PlantAi.h"                                        /* PlantAiのAPIと型定義 */
#include "PlantLog.h"                                       /* PlantLogのAPIと型定義 */
#include "PumpControl.h"                                    /* PumpControlのAPIと型定義 */
#include "SensorManager.h"                                  /* SensorManagerのAPIと型定義 */
#include "ConsoleUart.h"                                    /* ConsoleUartのAPIと型定義 */
#include "TimeKeeper.h"                                     /* TimeKeeperのAPIと型定義 */
#include "SoilCalibration.h"                                /* SoilCalibrationのAPIと型定義 */
#include "RtcRx4111.h"                                      /* RTC (RX4111CE) のAPIと型定義 */
#include "EbmlI2cTelemetry.h"                               /* ATOMS3 Lite向けI2Cテレメトリ送信 */
#include <stddef.h>                                         /* NULL定義 */

/** =================================================================*
 * @brief  診断情報コールバック
 * ================================================================= */
static bool App_GetDiagnosisCallback(uint8_t *stressScore, uint8_t *status, const char **soilTrend) {
    if (stressScore != NULL) {
        *stressScore = PlantAi_GetStressScore();
    }
    if (status != NULL) {
        *status = (uint8_t)PlantAi_GetStatus();
    }
    if (soilTrend != NULL) {
        SOIL_TREND trend = PlantAi_GetSoilTrend();
        switch (trend) {
            case SOIL_TREND_DRY:         *soilTrend = "DRYING";   break;
            case SOIL_TREND_WET:         *soilTrend = "WETTING";  break;
            case SOIL_TREND_DEGRADATION: *soilTrend = "DEGRADED"; break;
            case SOIL_TREND_STABLE:
            case SOIL_TREND_UNKNOWN:
            default:                     *soilTrend = "STABLE";   break;
        }
    }
    return true;
}

static bool App_SetUnixSecondsCallback(uint32_t unixSeconds) {
    TimeKeeper_Resynchronize(unixSeconds);
    RTC_RX4111_TIME rtcTime;
    RtcRx4111_UnixToTime(unixSeconds, &rtcTime);
    return RtcRx4111_SetTime(&rtcTime);
}

static bool App_GetCalibrationCallback(uint16_t *dry, uint16_t *wet) {
    SoilCalibration_Get(dry, wet);
    return true;
}

static uint8_t App_TriggerWateringCallback(void) {
    return (uint8_t)PumpControl_Request(true);
}

static const CONSOLE_SERVICES s_consoleServices = {
    .getRecordCount = PlantLog_GetRecordCount,
    .readRecord = PlantLog_ReadRecord,
    .eraseLog = PlantLog_Erase,
    .getUnixSeconds = TimeKeeper_GetUnixSeconds,
    .setUnixSeconds = App_SetUnixSecondsCallback,
    .getSnapshot = SensorManager_GetLatest,
    .getDiagnosis = App_GetDiagnosisCallback,
    .getCalibration = App_GetCalibrationCallback,
    .setCalibration = SoilCalibration_Set,
    .getMaxPendingTicks = Board_GetMaxPendingTicks,
    .setDemoMode = AppStateMachine_SetDemoMode,
    .triggerWatering = App_TriggerWateringCallback,
    .isPumpOn = PumpControl_IsOn
};

/** =================================================================*
 * @brief  App_Init処理
 * ================================================================= */
void App_Init(void) {
    PLANT_DOCTOR_ERROR error;

    AppStateMachine_Init();
    error = Board_Init();
    if (error != PLANT_DOCTOR_ERROR_NONE) {
        AppStateMachine_EnterError(error);
        return;
    }

    RtcRx4111_Init();
    RTC_RX4111_TIME bootRtcTime;
    if (RtcRx4111_GetTime(&bootRtcTime)) {
        uint32_t unixSec = RtcRx4111_TimeToUnix(&bootRtcTime);
        TimeKeeper_Init(unixSec, true);
    }

    if (!SensorManager_Init()) {
        AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_SENSOR_INTERFACE);
        return;
    }
    if (!PlantAi_Init()) {
        AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_AI);
        return;
    }
    if (!PumpControl_Init()) {
        AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_ACTUATOR);
        return;
    }
    if (!PlantLog_Init()) {
        AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_STORAGE_INTERFACE);
        return;
    }
    (void)ConsoleUart_Init(&s_consoleServices);
    (void)EbmlI2cTelemetry_Init();
}

/** =================================================================*
 * @brief  App_RunOnce処理
 * ================================================================= */
void App_RunOnce(void) {
    Board_ServiceWatchdog();

    if (Board_TakeTickOverflow()) {
        AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_TICK_OVERFLOW);
    }

    while (Board_Take10MsTick()) {
        if (!Board_Process10Ms()) {
            AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_SWITCH);
        }
        if (AppStateMachine_GetState() != APP_STATE_ERROR) {
            SensorManager_Process10Ms();
            PlantAi_Process10Ms();
            EbmlI2cTelemetry_Process10Ms();
        }
        PumpControl_Process10Ms();
        PlantLog_Process10Ms();
        ConsoleUart_Process10Ms();
        AppStateMachine_Tick10Ms();
    }

    AppStateMachine_Process();
}
