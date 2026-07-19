/*****************************************************************************
 * File: main.c
 * Title: Lcdの制御を行うプロジェクト。
 * LastUpdated: 2025.05.30
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief Lcdの制御を行うプロジェクト。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "SystemPowerControl.h"
#include "TimeControl.h"
#include "Lcd.h"

static void systemInit(void);

int main(void)
{
	const char* firstLineData = "DT-EBML63Q2557";
	const char* secondLineData = "DATA TECNO";
	
	systemInit();

	LcdDisplayOnOff(LCD_DISPLAY_ON,LCD_CURSOR_OFF,LCD_CURSOR_BLINK_OFF);
	LcdDraw(LCD_START_OF_FIRST_LINE,firstLineData);
	LcdDraw(LCD_START_OF_SECOND_LINE,secondLineData);
	
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
	
	__enable_irq();
	
	SystemPowerControlInit();
	TimeControlInit();

	LcdPeripheralInit();
	LcdInit();
}
