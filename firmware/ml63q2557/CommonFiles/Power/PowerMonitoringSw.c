/*****************************************************************************
 * File: PowerMonitoringSw.c
 * Title: 電源監視用SW(POWSW_CHK)を取り扱う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PowerMonitoringSw.c
 * @brief 電源監視用SW(POWSW_CHK)を取り扱う。
 */

#include <stdint.h>
#include "PowerMonitoringSw.h"
#include "smpl_common.h"
#include "Output.h"
#include "wdt.h"

#define POWER_MONITORING_SHIFT 	(4)
#define POWER_MONITORING_PUSH	(1 << POWER_MONITORING_SHIFT)
#define POWER_MONITORING_CONFIG	(0x09U << 0)
#define WAITING_TIME			(50)
#define PULLUP					(0x10)
#define SOFT_PULLDOWN			(0x00)

volatile static bool keepPowerMonitoringSwPressedFlg = false;

/**
 * @brief 入力値を読み取る関数
 *
 * @return uint8_t 
 */
inline static uint8_t readPowerMonitoringSw(void);

/**
 * @brief 入力値の変化に応じてフラグ管理を行う。
 */
inline static void managePowerMonitoringSwFlg(void);



void PowerMonitoringSwInit(void)
{
	set_reg32(PORT4->P4MOD1,POWER_MONITORING_CONFIG);
	InputInit(INPUT_INDEX_POWER_MONITORING,SOFT_PULLDOWN);
	keepPowerMonitoringSwPressedFlg = false;
}

INPUT_POLLING_RESULT PowerMonitoringSwPolling(void)
{
	return InputPolling(INPUT_INDEX_POWER_MONITORING,WAITING_TIME,readPowerMonitoringSw,managePowerMonitoringSwFlg);
}

bool PowerMonitoringSwIsPressed(void)
{
	return keepPowerMonitoringSwPressedFlg;
}



inline static uint8_t readPowerMonitoringSw(void)
{
	return (uint8_t) ( ( PORT4->P4DI ) & POWER_MONITORING_PUSH );
}

inline static void managePowerMonitoringSwFlg(void)
{
	//PullUpなので逆
	if( ! ( InputGetSystemInputValue(INPUT_INDEX_POWER_MONITORING) & POWER_MONITORING_PUSH) )
	{	
		keepPowerMonitoringSwPressedFlg = true;
	}
	else
	{
		keepPowerMonitoringSwPressedFlg = false;
	}	
}



