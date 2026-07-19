/*****************************************************************************
 * File: main.c
 * Title: Framの制御を行うプロジェクト。
 * LastUpdated: 2025.05.29
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief Framの制御を行うプロジェクト。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "SystemPowerControl.h"
#include "SoftSpi.h"
#include "Fram.h"
#include <stdbool.h>

static void systemInit(void);

int main(void)
{
	const uint8_t write[10] = {0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E, 0x30, 0x32};
	const uint32_t testAddressTop = 200000; 
	uint8_t readBuf[20];
	uint16_t read16;
	uint32_t read32;
	bool isFailed = false;
	
	systemInit();
	
	//framに書き込み
	FramWriteBlock(testAddressTop + 0, write, 10);
	FramWriteByte(testAddressTop + 10, 0x01);
	FramWriteByte(testAddressTop + 11, 0x02);
	FramWriteHalfWord(testAddressTop + 12, 0x0304);
	FramWriteWord(testAddressTop + 14, 0x05060708);
	
	//framから読み込み
	FramReadBlock(testAddressTop + 0, (uint8_t*)&readBuf[0], 20);
	for(int i = 0; i < 10; i++)
	{
		if(readBuf[i] != write[i]) isFailed = true;
	}
	
	FramReadByte(testAddressTop + 11, (uint8_t*)&readBuf[0]);
	if(readBuf[0] != 0x02) isFailed = true;	
	FramReadHalfWord(testAddressTop + 12, &read16);
	if(read16 != 0x0304) isFailed = true;		
	FramReadWord(testAddressTop + 14, &read32);
	if(read32 != 0x05060708) isFailed = true;
	
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
	SoftSpiPeripheralInit();
	FramInit();
	
	__enable_irq();
}
