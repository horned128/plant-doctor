/*****************************************************************************
 * File: main.c
 * Title: 電源保持回路の制御を行うプロジェクト。
 * LastUpdated: 2025.05.29
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief 電源保持回路の制御を行うプロジェクト。\n
 *        このプロジェクトを参考に電源保持の管理を行ってください。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "PowerMonitoringSw.h"
#include "PowerKeep.h"
#include "SystemPowerControl.h"
#include "TimeControl.h"

#define WAIT_INIT		(800)
#define WAIT_POLLING	(10)

static void systemInit(void);

int main(void)
{
	systemInit();
	TimeControlDelayMs(WAIT_INIT);
	while(!PowerMonitoringSwIsPressed())
	{
		TimeControlDelayMs(WAIT_POLLING);
		PowerMonitoringSwPolling();	
		wdt_clear();
	}
	SystemPowerControlFin();
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
	TimeControlInit();
	
	__enable_irq();
}
