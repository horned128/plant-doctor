/*****************************************************************************
 * File: main.c
 * Title: 電源電圧監視を行うプロジェクト。
 * LastUpdated: 2025.05.29
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief 電源電圧監視を行うプロジェクト。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "irq.h"
#include "PowerMonitoringSw.h"
#include "PowerKeep.h"
#include "SystemPowerControl.h"
#include "PowerMonitoringAnalogInput.h"

static void systemInit(void);

int main(void)
{
	volatile float powerValue;
	
	systemInit();

	while(1)
	{
		powerValue = PowerMonitoringAnalogInputGetVoltageValue();
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
	PowerMonitoringAnalogInputInit();
	
	__enable_irq();
}
