/** =================================================================*
 * @file   AppStateMachine.c
 * @brief  アプリケーション状態機械
 * ================================================================= */
#include "AppStateMachine.h"                                /* AppStateMachineのAPIと型定義 */
#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "Board.h"                                          /* BoardのAPIと型定義 */
#include "LedControl.h"                                     /* LedControlのAPIと型定義 */
#include "LcdUi.h"                                          /* LcdUiのAPIと型定義 */
#include "PlantDoctorConfig.h"                              /* PlantDoctorConfigのAPIと型定義 */
#include "PumpControl.h"                                    /* PumpControlのAPIと型定義 */
#include "SensorManager.h"                                  /* センサー取得APIとスナップショット型 */
#include "SwitchControl.h"                                  /* SwitchControlのAPIと型定義 */
#include "PlantAi.h"                                        /* 植物状態AI・診断API */
#include "WateringPolicy.h"                                 /* 自律水やり判定ポリシーAPI */
#include "DemoMode.h"                                       /* デモモード基盤API */
#include "SoilCalibration.h"                                /* 土壌水分校正変換API */
#include "TankLevelSensor.h"                                /* タンク液面センサーAPI */
#include "TimeKeeper.h"                                     /* 時刻管理API */
#include "PlantLog.h"                                       /* 植物ログ保存API */
#include <stddef.h>                                         /* NULL定義 */

static APP_STATE s_state;                                   /**< モジュール内部状態 */
static PLANT_DOCTOR_ERROR s_error;                          /**< モジュール内部状態 */
static uint16_t s_stateTicks;                               /**< モジュール内部状態 */
static uint16_t s_ledTicks;                                 /**< モジュール内部状態 */
static uint16_t s_sensorDisplayTicks;                       /**< センサー画面切替までのTick */
static uint16_t s_sensorRefreshTicks;                       /**< 表示中センサー画面更新までのTick */
static uint16_t s_lcdRecoveryTicks;                         /**< 次のLCD復旧試行までのTick */
static uint8_t s_lcdRecoveryAttempts;                       /**< 連続LCD復旧試行回数 */
static uint8_t s_previousSwitchMask;                        /**< モジュール内部状態 */
static uint8_t s_requestedSwitchMask;                       /**< モジュール内部状態 */
static uint8_t s_sensorPage;                                /**< 表示中センサーページ */
static bool s_uiUpdatePending;                              /**< モジュール内部状態 */
static bool s_sensorDisplayPending;                         /**< センサー画面更新要求 */
static bool s_pumpMessagePending;                           /**< ポンプメッセージ表示要求 */
static const char *s_pumpMessage;                           /**< ポンプ表示メッセージ */
static uint16_t s_pumpUiHoldTicks;                          /**< ポンプUI表示維持Tick */
static bool s_lcdRecoveryActive;                            /**< LCD復旧処理中 */
static bool s_lcdRecoveryPending;                           /**< LCD復旧試行要求 */
static bool s_errorShown;                                   /**< モジュール内部状態 */
static bool s_errorLedPhase;                                /**< モジュール内部状態 */
static WATERING_POLICY_STATE s_wateringPolicyState;         /**< 自律水やり状態 */
static WATERING_POLICY_CONFIG s_wateringPolicyConfig;       /**< 自律水やり設定 */
static DEMO_MODE_STATE s_demoModeState;                     /**< デモモード状態 */
static uint32_t s_appTick;                                  /**< 10ms Tickカウンタ */

/** =================================================================*
 * @brief  AppStateMachine_SetState処理
 * @param[in] state 引数
 * ================================================================= */
static void AppStateMachine_SetState(APP_STATE state) {
    s_state = state;
    s_stateTicks = 0U;
    s_ledTicks = 0U;
    s_sensorDisplayTicks = 0U;
    s_sensorRefreshTicks = 0U;
    s_lcdRecoveryTicks = 0U;
    s_lcdRecoveryAttempts = 0U;
    s_sensorDisplayPending = (state == APP_STATE_MONITOR);
    s_pumpMessagePending = false;
    s_pumpMessage = 0;
    s_pumpUiHoldTicks = 0U;
    s_lcdRecoveryActive = false;
    s_lcdRecoveryPending = false;
}

/** =================================================================*
 * @brief  LCDの復旧処理を開始する。
 * ================================================================= */
static void AppStateMachine_StartLcdRecovery(void) {
    if (!s_lcdRecoveryActive) {
        s_lcdRecoveryActive = true;
        s_lcdRecoveryTicks = 0U;
        s_lcdRecoveryPending = false;
    }
}

/** =================================================================*
 * @brief  要求済みのLCD復旧を1回実行する。
 * @details 復旧不能時もシステムを停止させずMONITORを継続する。
 * ================================================================= */
static void AppStateMachine_ProcessLcdRecovery(void) {
    if (!s_lcdRecoveryPending) {
        return;
    }

    s_lcdRecoveryPending = false;
    if (s_lcdRecoveryAttempts >= PLANT_DOCTOR_LCD_RECOVERY_MAX_ATTEMPTS) {
        s_lcdRecoveryActive = false;
        return;
    }

    ++s_lcdRecoveryAttempts;
    if (LcdUi_Recover()) {
        s_lcdRecoveryActive = false;
        s_lcdRecoveryTicks = 0U;
        s_lcdRecoveryAttempts = 0U;
        s_uiUpdatePending = true;
        s_sensorDisplayPending = true;
    } else if (s_lcdRecoveryAttempts >= PLANT_DOCTOR_LCD_RECOVERY_MAX_ATTEMPTS) {
        s_lcdRecoveryActive = false;
    }
}

/** =================================================================*
 * @brief  AppStateMachine_Init処理
 * ================================================================= */
void AppStateMachine_Init(void) {
    s_error = PLANT_DOCTOR_ERROR_NONE;
    s_previousSwitchMask = 0U;
    s_requestedSwitchMask = 0U;
    s_sensorPage = 0U;
    s_uiUpdatePending = false;
    s_sensorDisplayPending = false;
    s_pumpMessagePending = false;
    s_pumpMessage = 0;
    s_pumpUiHoldTicks = 0U;
    s_errorShown = false;
    s_errorLedPhase = false;
    s_appTick = 0U;

    WateringPolicy_Reset(&s_wateringPolicyState);
    DemoMode_Reset(&s_demoModeState);
    TimeKeeper_Init(0xFFFFFFFFUL, false);
    s_wateringPolicyConfig.dryThresholdPermille = PLANT_DOCTOR_WATERING_DRY_THRESHOLD_PERMILLE;
    s_wateringPolicyConfig.minIntervalSeconds = PLANT_DOCTOR_WATERING_MIN_INTERVAL_SECONDS;
    s_wateringPolicyConfig.minIntervalTicks = PLANT_DOCTOR_WATERING_MIN_INTERVAL_TICKS;
    s_wateringPolicyConfig.maxDailyWateringCount = PLANT_DOCTOR_WATERING_MAX_DAILY_COUNT;
    s_wateringPolicyConfig.autoWateringEnabled = PLANT_DOCTOR_WATERING_AUTO_ENABLE;

    AppStateMachine_SetState(APP_STATE_BOOT);
}

/** =================================================================*
 * @brief  AppStateMachine_Process処理
 * ================================================================= */
void AppStateMachine_Process(void) {
    switch (s_state) {
        case APP_STATE_BOOT:
            if (!LcdUi_Init()) {
                AppStateMachine_StartLcdRecovery();
            } else if (!LcdUi_ShowBoardTest()) {
                AppStateMachine_StartLcdRecovery();
            }
            AppStateMachine_SetState(APP_STATE_SELF_TEST);
            break;

        case APP_STATE_SELF_TEST:
            if (!Board_IsPowerHeld()) {
                AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_POWER);
            } else if (s_stateTicks >= PLANT_DOCTOR_SELF_TEST_TICKS) {
                AppStateMachine_SetState(APP_STATE_MONITOR);
            }
            break;

        case APP_STATE_MONITOR:
            if (s_lcdRecoveryActive) {
                AppStateMachine_ProcessLcdRecovery();
            } else if (s_pumpMessagePending) {
                s_pumpMessagePending = false;
                if (!LcdUi_ShowPumpStatus(s_pumpMessage)) {
                    AppStateMachine_StartLcdRecovery();
                }
            } else if (s_uiUpdatePending) {
                s_uiUpdatePending = false;
                if (!LcdUi_ShowSwitch(s_requestedSwitchMask)) {
                    AppStateMachine_StartLcdRecovery();
                }
            } else if (s_sensorDisplayPending) {
                if (s_pumpUiHoldTicks == 0U) {
                    PLANT_SENSOR_SNAPSHOT snapshot;

                    if (SensorManager_GetLatest(&snapshot)) {
                        s_sensorDisplayPending = false;
                        LCD_DIAGNOSIS_VIEW_DATA diagData;
                        PLANT_FEATURE_VECTOR feat;

                        PlantAi_GetFeatureVector(&feat);
                        diagData.stressScore = PlantAi_GetStressScore();
                        diagData.status = PlantAi_GetStatus();
                        diagData.failedSensor = PlantAi_GetFailedSensor();
                        diagData.soilTrend = PlantAi_GetSoilTrend();
                        diagData.leafAirTemperatureDelta = feat.leafAirTemperatureDelta;
                        diagData.leafAirDeltaValid = ((feat.validMask & PLANT_FEATURE_VALID_LEAF_AIR_DELTA) != 0U);
                        diagData.isDemoMode = DemoMode_IsActive(&s_demoModeState);

                        if (!LcdUi_ShowDiagnosisPage(&diagData, &snapshot, s_sensorPage)) {
                            AppStateMachine_StartLcdRecovery();
                        } else {
                            s_lcdRecoveryAttempts = 0U;
                        }
                    }
                }
            }
            break;

        case APP_STATE_ERROR:
            if (!s_errorShown) {
                if (!LcdUi_IsReady()) {
                    if ((s_stateTicks == 0U) || ((s_stateTicks % PLANT_DOCTOR_SELF_TEST_TICKS) == 0U)) {
                        (void)LcdUi_Init();
                    }
                }
                if (LcdUi_IsReady()) {
                    s_errorShown = LcdUi_ShowError(s_error);
                }
            }
            break;

        default:
            AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_NONE);
            break;
    }
}

/** =================================================================*
 * @brief  AppStateMachine_Tick10Ms処理
 * ================================================================= */
void AppStateMachine_Tick10Ms(void) {
    PUMP_WATERING_EVENT wateringEvent;
    uint8_t switchMask;

    if (s_stateTicks < UINT16_MAX) {
        ++s_stateTicks;
    }
    ++s_appTick;
    TimeKeeper_Tick10Ms();

    /* 給水完了イベントを取り出してAIおよび自律ポリシー、ログへ通知 */
    if (PumpControl_TakeWateringEvent(&wateringEvent)) {
        PlantAi_NotifyWatering(&wateringEvent, s_appTick);
        WateringPolicy_NotifyWateringExecuted(&s_wateringPolicyState,
                                              wateringEvent.startTimeSeconds,
                                              s_appTick);
        PlantLog_NotifyWatering(&wateringEvent);
    }

    if (s_state == APP_STATE_MONITOR) {
        ++s_ledTicks;
        if (s_ledTicks >= PLANT_DOCTOR_LED_BLINK_TICKS) {
            s_ledTicks = 0U;
            LedControl_Toggle(LED_CONTROL_1);
        }
        ++s_sensorDisplayTicks;
        if (s_sensorDisplayTicks >= PLANT_DOCTOR_SENSOR_DISPLAY_TICKS) {
            s_sensorDisplayTicks = 0U;
            s_sensorPage = (uint8_t)((s_sensorPage + 1U) % 3U);
            s_sensorDisplayPending = true;
        }
        ++s_sensorRefreshTicks;
        if (s_sensorRefreshTicks >= PLANT_DOCTOR_SENSOR_SAMPLE_TICKS) {
            s_sensorRefreshTicks = 0U;
            s_sensorDisplayPending = true;
        }
        if (s_lcdRecoveryActive) {
            ++s_lcdRecoveryTicks;
            if (s_lcdRecoveryTicks >= PLANT_DOCTOR_LCD_RECOVERY_INTERVAL_TICKS) {
                s_lcdRecoveryTicks = 0U;
                s_lcdRecoveryPending = true;
            }
        }

        if (s_pumpUiHoldTicks > 0U) {
            --s_pumpUiHoldTicks;
            if (s_pumpUiHoldTicks == 0U) {
                s_sensorDisplayPending = true;
            }
        }

        switchMask = Board_GetPressedSwitchMask();

        /* SW1 長押し/短押しによるデモモード処理 */
        DemoMode_ProcessSwitch1Tick(&s_demoModeState,
                                    (switchMask & SWITCH_CONTROL_PSW1) != 0U,
                                    PLANT_DOCTOR_DEMO_SW1_HOLD_TICKS);

        if (switchMask != s_previousSwitchMask) {
            uint8_t pressedEdge = switchMask & (uint8_t)(~s_previousSwitchMask);

            s_previousSwitchMask = switchMask;
            if ((pressedEdge & SWITCH_CONTROL_PSW4) != 0U) {
                if (PumpControl_IsOn()) {
                    (void)PumpControl_Request(false);
                    s_pumpMessage = "PUMP STOPPED";
                    s_pumpUiHoldTicks = PLANT_DOCTOR_PUMP_MESSAGE_TICKS;
                } else {
                    PLANT_SENSOR_SNAPSHOT snapshot;
                    uint32_t nowSec = 0xFFFFFFFFU;
                    uint16_t soilRaw = 0U;
                    int16_t soilPermille = 0;
                    int16_t leafAirDelta = 0;
                    PUMP_CONTROL_STATUS status;

                    if (SensorManager_GetLatest(&snapshot)) {
                        nowSec = snapshot.timestampSeconds;
                        soilRaw = snapshot.soilMoistureRaw;
                        soilPermille = SoilCalibration_ToPermille(snapshot.soilMoistureRaw,
                                                                 PLANT_DOCTOR_SOIL_CAL_DEFAULT_DRY,
                                                                 PLANT_DOCTOR_SOIL_CAL_DEFAULT_WET);
                        leafAirDelta = (int16_t)(snapshot.leafTemperatureCentiC - snapshot.airTemperatureCentiC);
                    }
                    PumpControl_SetPreWateringContext(nowSec, soilRaw, soilPermille, leafAirDelta, false);
                    status = PumpControl_Request(true);

                    if (status == PUMP_CONTROL_STATUS_OK) {
                        s_pumpMessage = "WATERING 2.0s";
                        s_pumpUiHoldTicks = PLANT_DOCTOR_PUMP_MAX_ON_TICKS;
                    } else if (status == PUMP_CONTROL_STATUS_EMPTY) {
                        s_pumpMessage = "TANK EMPTY!";
                        s_pumpUiHoldTicks = PLANT_DOCTOR_PUMP_MESSAGE_TICKS;
                    } else if (status == PUMP_CONTROL_STATUS_COOLDOWN) {
                        s_pumpMessage = "PUMP COOLDOWN";
                        s_pumpUiHoldTicks = PLANT_DOCTOR_PUMP_MESSAGE_TICKS;
                    } else {
                        s_pumpMessage = "PUMP ERROR";
                        s_pumpUiHoldTicks = PLANT_DOCTOR_PUMP_MESSAGE_TICKS;
                    }
                }
                s_pumpMessagePending = true;
            } else if ((switchMask & (SWITCH_CONTROL_PSW1 | SWITCH_CONTROL_PSW2 | SWITCH_CONTROL_PSW3)) != 0U) {
                s_requestedSwitchMask = switchMask;
                s_uiUpdatePending = true;
            } else if (switchMask == 0U) {
                s_requestedSwitchMask = 0U;
                s_uiUpdatePending = true;
            }
        }

        /* 自律水やり判定 (給水動作中でなく、UI表示ホールドも解けている場合) */
        if (!PumpControl_IsOn() && (s_pumpUiHoldTicks == 0U)) {
            PLANT_SENSOR_SNAPSHOT snapshot;
            if (SensorManager_GetLatest(&snapshot)) {
                WATERING_POLICY_INPUT policyInput;
                int16_t soilPermille = SoilCalibration_ToPermille(
                    snapshot.soilMoistureRaw,
                    PLANT_DOCTOR_SOIL_CAL_DEFAULT_DRY,
                    PLANT_DOCTOR_SOIL_CAL_DEFAULT_WET);
                SENSOR_HEALTH soilHealth = snapshot.soilMoistureValid ? SENSOR_HEALTH_OK : SENSOR_HEALTH_NO_COMMUNICATION;
                bool tankLiquid = TankLevelSensor_IsLiquidDetected();

                if (DemoMode_IsActive(&s_demoModeState)) {
                    DemoMode_ApplyOffsets(&s_demoModeState,
                                          NULL,
                                          &soilPermille,
                                          NULL,
                                          &tankLiquid,
                                          &soilHealth);
                }

                policyInput.plantStatus = PlantAi_GetStatus();
                policyInput.soilMoisturePermille = soilPermille;
                policyInput.soilSensorHealth = soilHealth;
                policyInput.tankLiquidDetected = tankLiquid;
                policyInput.isMonitoring = (s_state == APP_STATE_MONITOR);
                policyInput.lastWateringResponse = PlantAi_GetWateringResponse();
                policyInput.nowSeconds = snapshot.timestampSeconds;
                policyInput.currentTick = s_appTick;

                if (WateringPolicy_Evaluate(&s_wateringPolicyState, &policyInput, &s_wateringPolicyConfig) == WATERING_DECISION_REQUEST) {
                    int16_t leafAirDelta = (int16_t)(snapshot.leafTemperatureCentiC - snapshot.airTemperatureCentiC);
                    PumpControl_SetPreWateringContext(snapshot.timestampSeconds,
                                                      snapshot.soilMoistureRaw,
                                                      soilPermille,
                                                      leafAirDelta,
                                                      true);
                    if (PumpControl_Request(true) == PUMP_CONTROL_STATUS_OK) {
                        s_pumpMessage = "AUTO WATERING";
                        s_pumpUiHoldTicks = PLANT_DOCTOR_PUMP_MAX_ON_TICKS;
                        s_pumpMessagePending = true;
                    }
                }
            }
        }

        LedControl_Set(LED_CONTROL_2, (switchMask & 0x03U) != 0U);
        LedControl_Set(LED_CONTROL_3, ((switchMask & SWITCH_CONTROL_PSW3) != 0U) || PumpControl_IsOn());
    } else if (s_state == APP_STATE_ERROR) {
        ++s_ledTicks;
        if (s_ledTicks >= PLANT_DOCTOR_ERROR_BLINK_TICKS) {
            s_ledTicks = 0U;
            s_errorLedPhase = !s_errorLedPhase;
            LedControl_Set(LED_CONTROL_1, s_errorLedPhase);
            LedControl_Set(LED_CONTROL_2, !s_errorLedPhase);
            LedControl_Set(LED_CONTROL_3, s_errorLedPhase);
        }
    }
}

/** =================================================================*
 * @brief  AppStateMachine_EnterError処理
 * @param[in] error 引数
 * ================================================================= */
void AppStateMachine_EnterError(PLANT_DOCTOR_ERROR error) {
    PumpControl_EmergencyStop();
    s_error = error;
    s_errorShown = false;
    s_errorLedPhase = false;
    LedControl_AllOff();
    PlantLog_NotifyError(error);
    AppStateMachine_SetState(APP_STATE_ERROR);
}

/** =================================================================*
 * @brief  AppStateMachine_GetState処理
 * @return 実行結果または取得値
 * ================================================================= */
APP_STATE AppStateMachine_GetState(void) {
    return s_state;
}

/** =================================================================*
 * @brief  AppStateMachine_GetError処理
 * @return 実行結果または取得値
 * ================================================================= */
PLANT_DOCTOR_ERROR AppStateMachine_GetError(void) {
    return s_error;
}

/** =================================================================*
 * @brief  AppStateMachine_SetDemoMode処理
 * @param[in] enable 有効/無効
 * @return 実行結果
 * ================================================================= */
bool AppStateMachine_SetDemoMode(bool enable) {
    DemoMode_SetActive(&s_demoModeState, enable);
    if (enable) {
        DemoMode_SetScenario(&s_demoModeState, DEMO_SCENARIO_1_NORMAL);
    } else {
        DemoMode_SetScenario(&s_demoModeState, DEMO_SCENARIO_OFF);
    }
    return true;
}
