/*****************************************************************************
 * File: SystemPowerControl.c
 * Title: 電源関係(POWER_KEEP、POWSW_CHK)の制御。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file SystemPowerControl.c
 * @brief 電源関係(POWER_KEEP、POWSW_CHK)の制御。
 */

#include "SystemPowerControl.h"
#include "PowerMonitoringSw.h"
#include "PowerKeep.h"

void SystemPowerControlInit(void)
{
	PowerKeepInit();
	PowerMonitoringSwInit();
	PowerKeepOn();
}

void SystemPowerControlFin(void)
{
	PowerKeepOff();
}
