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

    if (!SensorManager_Init() || !PlantAi_Init() || !PumpControl_Init()) {
        AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_SENSOR_INTERFACE);
        return;
    }
    if (!PlantLog_Init()) {
        AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_STORAGE_INTERFACE);
    }
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
        }
        PumpControl_Process10Ms();
        PlantLog_Process10Ms();
        AppStateMachine_Tick10Ms();
    }

    AppStateMachine_Process();
}
