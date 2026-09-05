#ifndef APP_STATE_MACHINE_H
#define APP_STATE_MACHINE_H

#include "PlantDoctorStatus.h"

typedef enum
{
	APP_STATE_BOOT = 0,
	APP_STATE_SELF_TEST,
	APP_STATE_MONITOR,
	APP_STATE_ERROR
} APP_STATE;

void AppStateMachine_Init(void);
void AppStateMachine_Process(void);
void AppStateMachine_Tick10Ms(void);
void AppStateMachine_EnterError(PLANT_DOCTOR_ERROR error);
APP_STATE AppStateMachine_GetState(void);
PLANT_DOCTOR_ERROR AppStateMachine_GetError(void);

#endif /* APP_STATE_MACHINE_H */
