#include "Board.h"

#include "BoardTimer.h"
#include "LedControl.h"
#include "PowerControlAdapter.h"
#include "SwitchControl.h"
#include "clock.h"
#include "mcu.h"
#include "smpl_common.h"
#include "wdt.h"

PLANT_DOCTOR_ERROR Board_Init(void)
{
	PLANT_DOCTOR_ERROR error = PLANT_DOCTOR_ERROR_NONE;

	__disable_irq();
	wdt_init(WDT_2S);
	wdt_clear();
	smpl_setLsCrystal32Khz();
	smpl_setHsPll48Mhz(CLK_XSPEN_DIS, CLK_HXSPEN_DIS);

	if (!PowerControlAdapter_Init())
	{
		error = PLANT_DOCTOR_ERROR_POWER;
	}
	LedControl_Init();
	if (!SwitchControl_Init() && (error == PLANT_DOCTOR_ERROR_NONE))
	{
		error = PLANT_DOCTOR_ERROR_SWITCH;
	}
	if (!BoardTimer_Init() && (error == PLANT_DOCTOR_ERROR_NONE))
	{
		error = PLANT_DOCTOR_ERROR_TIMER;
	}
	__enable_irq();

	return error;
}

bool Board_Process10Ms(void)
{
	return SwitchControl_Process10Ms();
}

bool Board_Take10MsTick(void)
{
	return BoardTimer_Take10MsTick();
}

bool Board_TakeTickOverflow(void)
{
	return BoardTimer_TakeOverflow();
}

uint8_t Board_GetPressedSwitchMask(void)
{
	return SwitchControl_GetPressedMask();
}

bool Board_IsPowerHeld(void)
{
	return PowerControlAdapter_IsPowerHeld();
}

void Board_ServiceWatchdog(void)
{
	wdt_clear();
}
