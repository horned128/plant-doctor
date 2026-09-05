#include "AppStateMachine.h"

#include <stdbool.h>
#include <stdint.h>

#include "Board.h"
#include "LedControl.h"
#include "LcdUi.h"
#include "PlantDoctorConfig.h"

static APP_STATE s_state;
static PLANT_DOCTOR_ERROR s_error;
static uint16_t s_stateTicks;
static uint16_t s_ledTicks;
static uint8_t s_previousSwitchMask;
static uint8_t s_requestedSwitchMask;
static bool s_uiUpdatePending;
static bool s_errorShown;
static bool s_errorLedPhase;

static void AppStateMachine_SetState(APP_STATE state)
{
	s_state = state;
	s_stateTicks = 0U;
	s_ledTicks = 0U;
}

void AppStateMachine_Init(void)
{
	s_error = PLANT_DOCTOR_ERROR_NONE;
	s_previousSwitchMask = 0U;
	s_requestedSwitchMask = 0U;
	s_uiUpdatePending = false;
	s_errorShown = false;
	s_errorLedPhase = false;
	AppStateMachine_SetState(APP_STATE_BOOT);
}

void AppStateMachine_Process(void)
{
	switch (s_state)
	{
		case APP_STATE_BOOT:
			if (!LcdUi_Init())
			{
				AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_LCD_INIT);
			}
			else if (!LcdUi_ShowBoardTest())
			{
				AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_LCD_IO);
			}
			else
			{
				AppStateMachine_SetState(APP_STATE_SELF_TEST);
			}
			break;

		case APP_STATE_SELF_TEST:
			if (!Board_IsPowerHeld())
			{
				AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_POWER);
			}
			else if (s_stateTicks >= PLANT_DOCTOR_SELF_TEST_TICKS)
			{
				AppStateMachine_SetState(APP_STATE_MONITOR);
			}
			break;

		case APP_STATE_MONITOR:
			if (s_uiUpdatePending)
			{
				s_uiUpdatePending = false;
				if (!LcdUi_ShowSwitch(s_requestedSwitchMask))
				{
					AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_LCD_IO);
				}
			}
			break;

		case APP_STATE_ERROR:
			if (!s_errorShown)
			{
				s_errorShown = true;
				if (LcdUi_IsReady())
				{
					(void)LcdUi_ShowError(s_error);
				}
			}
			break;

		default:
			AppStateMachine_EnterError(PLANT_DOCTOR_ERROR_NONE);
			break;
	}
}

void AppStateMachine_Tick10Ms(void)
{
	uint8_t switchMask;

	if (s_stateTicks < UINT16_MAX)
	{
		++s_stateTicks;
	}

	if (s_state == APP_STATE_MONITOR)
	{
		++s_ledTicks;
		if (s_ledTicks >= PLANT_DOCTOR_LED_BLINK_TICKS)
		{
			s_ledTicks = 0U;
			LedControl_Toggle(LED_CONTROL_1);
		}

		switchMask = Board_GetPressedSwitchMask();
		if (switchMask != s_previousSwitchMask)
		{
			s_previousSwitchMask = switchMask;
			s_requestedSwitchMask = switchMask;
			s_uiUpdatePending = true;
		}
		LedControl_Set(LED_CONTROL_2, (switchMask & 0x03U) != 0U);
		LedControl_Set(LED_CONTROL_3, (switchMask & 0x0CU) != 0U);
	}
	else if (s_state == APP_STATE_ERROR)
	{
		++s_ledTicks;
		if (s_ledTicks >= PLANT_DOCTOR_ERROR_BLINK_TICKS)
		{
			s_ledTicks = 0U;
			s_errorLedPhase = !s_errorLedPhase;
			LedControl_Set(LED_CONTROL_1, s_errorLedPhase);
			LedControl_Set(LED_CONTROL_2, !s_errorLedPhase);
			LedControl_Set(LED_CONTROL_3, s_errorLedPhase);
		}
	}
}

void AppStateMachine_EnterError(PLANT_DOCTOR_ERROR error)
{
	s_error = error;
	s_errorShown = false;
	s_errorLedPhase = false;
	LedControl_AllOff();
	AppStateMachine_SetState(APP_STATE_ERROR);
}

APP_STATE AppStateMachine_GetState(void)
{
	return s_state;
}

PLANT_DOCTOR_ERROR AppStateMachine_GetError(void)
{
	return s_error;
}
