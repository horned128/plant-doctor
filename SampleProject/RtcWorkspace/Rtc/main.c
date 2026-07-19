/*****************************************************************************
 * File: main.c
 * Title: Rtcの制御を行うプロジェクト。
 * LastUpdated: 2025.05.29
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief Rtcの制御を行うプロジェクト。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "SystemPowerControl.h"
#include "SoftSpi.h"
#include "RX4111.h"

static void systemInit(void);

int main(void)
{
	const RX4111_DATE_TIME setTime = {55,5,5,5,55,55};
	RX4111_DATE_TIME getTime = {0,0,0,0,0,0};
	
	systemInit();
	
	RX4111SetTime(&setTime);

	while(1)
	{
		RX4111GetTime(&getTime);
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
	SoftSpiPeripheralInit();
	RX4111Init();
	
	__enable_irq();
}
