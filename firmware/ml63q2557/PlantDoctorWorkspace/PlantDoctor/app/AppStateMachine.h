/** =================================================================*
 * @file   AppStateMachine.h
 * @brief  アプリケーション状態機械API
 * ================================================================= */
#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

#include "PlantDoctorStatus.h"                              /* PlantDoctorStatusのAPIと型定義 */

typedef enum {
    APP_STATE_BOOT = 0,
    APP_STATE_SELF_TEST,
    APP_STATE_MONITOR,
    APP_STATE_ERROR
} APP_STATE;

void AppStateMachine_Init(void);                            /* AppStateMachine_InitのAPI */
void AppStateMachine_Process(void);                         /* AppStateMachine_ProcessのAPI */
void AppStateMachine_Tick10Ms(void);                        /* AppStateMachine_Tick10MsのAPI */
void AppStateMachine_EnterError(PLANT_DOCTOR_ERROR error);  /* AppStateMachine_EnterErrorのAPI */
APP_STATE AppStateMachine_GetState(void);                   /* AppStateMachine_GetStateのAPI */
PLANT_DOCTOR_ERROR AppStateMachine_GetError(void);          /* AppStateMachine_GetErrorのAPI */

#endif /* APP_STATE_MACHINE_H */
