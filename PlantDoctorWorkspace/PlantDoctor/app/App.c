#include "App.h"

#include "AppStateMachine.h"
#include "Board.h"
#include "PlantAi.h"
#include "PlantLog.h"
#include "PumpControl.h"
#include "SensorManager.h"

void App_Init(void)
{
	PLANT_DOCTOR_ERROR error;

	AppStateMachine_Init();
	error = Board_Init();
	if (error != PLANT_DOCTOR_ERROR_NONE)
	{
		AppStateMachine_EnterError(error);
		return;
	}

	if (!SensorManager_Init() || !PlantAi_Init() || !PumpControl_Init())
	{
		AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_SENSOR_INTERFACE);
		return;
	}
	if (!PlantLog_Init())
	{
		AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_STORAGE_INTERFACE);
	}
}

void App_RunOnce(void)
{
	Board_ServiceWatchdog();

	if (Board_TakeTickOverflow())
	{
		AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_TICK_OVERFLOW);
	}

	while (Board_Take10MsTick())
	{
		if (!Board_Process10Ms())
		{
			AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_SWITCH);
		}
		SensorManager_Process10Ms();
		PlantAi_Process10Ms();
		PlantLog_Process10Ms();
		AppStateMachine_Tick10Ms();
	}

	AppStateMachine_Process();
}
