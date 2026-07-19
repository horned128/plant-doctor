/*****************************************************************************
 * File: Regulator24VOutput.c
 * Title: 24Vレギュレータを制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Regulator24VOutput.c
 * @brief 24Vレギュレータを制御する。
 */

#include "Regulator24VOutput.h"
#include "smpl_common.h"

//P47
#define REGULATOR_24V_CONFIG	(0x02 << 24)
#define REGULATOR_24V_VALUE		(0x80)

static OUTPUT_STATUS outputStatus = OUTPUT_STATUS_OFF;

void Regulator24VOutputInit(void)
{
	set_bit(PORT4->P4MOD1, REGULATOR_24V_CONFIG);
	Regulator24VOutputOff();
}

void Regulator24VOutputOn(void)
{
	outputStatus = OutputOnUInt32(&(PORT4->P4DO),REGULATOR_24V_VALUE);
}

void Regulator24VOutputOff(void)
{
	outputStatus = OutputOffUInt32(&(PORT4->P4DO),REGULATOR_24V_VALUE);
}

OUTPUT_STATUS Regulator24VOutputGetOutputStatus(void)
{
	return outputStatus;
}
