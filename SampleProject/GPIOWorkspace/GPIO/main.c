/*****************************************************************************
 * File: main.c
 * Title: GPIOの制御を行うプロジェクト。
 * LastUpdated: 2025.05.29
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief GPIOの制御を行うプロジェクト。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "SystemPowerControl.h"
#include "TimeControl.h"
#include "PhotoCouplerInput.h"
#include "Sw.h"
#include "smpl_common_led.h"
#include "Regulator5VOutput.h"
#include "Regulator24VOutput.h"
#include "RelayOutput.h"

static void pollingGPIO(void);
static void systemInit(void);

int main(void)
{
	systemInit();
	
	while(1)
	{
		wdt_clear();
		TimeControlDelayMs(10);
		pollingGPIO();
		
		//プッシュスイッチ入力とLED出力
		if(SwIsPsw1Entered()) smpl_onLED1();
		else smpl_offLED1();
		if(SwIsPsw2Entered()) smpl_onLED2();
		else smpl_offLED2();
		if(SwIsPsw3Entered()) smpl_onLED3();
		else smpl_offLED3();
		
		//フォトカプラ入力と出力
		if(PhotoCouplerInput0IsEntered())RelayOutputRelay0On();
		else RelayOutputRelay0Off();
		if(PhotoCouplerInput1IsEntered())RelayOutputRelay1On();
		else RelayOutputRelay1Off();
		
		//ディップスイッチ1と5Vレギュレータ制御
		if(SwIsDsw1Entered())Regulator5VOutputOn();
		else Regulator5VOutputOff();
		//ディップスイッチ2と24Vレギュレータ制御
		if(SwIsDsw2Entered())Regulator24VOutputOn();
		else Regulator24VOutputOff();
	}
}

static void pollingGPIO(void)
{
	SwPolling();
	PhotoCouplerInputPolling();
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
	
	PhotoCouplerInputInit();
	SwInit();
	
	smpl_initLED1(LED_INACTIVE);
	smpl_initLED2(LED_INACTIVE);
	smpl_initLED3(LED_INACTIVE);
	RelayOutputInit();
	Regulator5VOutputInit();
	Regulator24VOutputInit();
	
	TimeControlInit();
}
