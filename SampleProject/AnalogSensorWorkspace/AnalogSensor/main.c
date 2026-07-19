/*****************************************************************************
 * File: main.c
 * Title: アナログセンサー用ADCの制御を行うプロジェクト。
 * LastUpdated: 2025.05.29
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief アナログセンサー用ADCの制御を行うプロジェクト。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "SystemPowerControl.h"
#include "AnalogSensor.h"
#include <string.h>

static void systemInit(void);

int main(void)
{
	int16_t debugBuffer[ANALOG_SENSOR_BUFFER_SIZE];
	int16_t* memory;
	ANALOG_SENSOR_BUFFER_STATE state;

	systemInit();

	memset(debugBuffer,0,sizeof(debugBuffer));
	AnalogSensorStart();

	while(AnalogSensorStartUsingSensorBuffer(&memory) != ANALOG_SENSOR_BUFFER_STATE_IS_VALID)
	{
		wdt_clear();
	}

	memcpy(debugBuffer,memory,sizeof(debugBuffer));

	state = AnalogSensorStopUsingSensorBuffer();
	AnalogSensorStop();

	while(1)
	{
		wdt_clear();
	}
}

static void systemInit(void)
{
	__disable_irq();

	wdt_init( WDT_2S );
	wdt_clear();
	smpl_setLsCrystal32Khz();
	smpl_setHsPll48Mhz( CLK_XSPEN_DIS,CLK_HXSPEN_DIS );
	SystemPowerControlInit();
	AnalogSensorDefaultInit();

	__enable_irq();
}
