/*****************************************************************************
 * File: Regulator5VOutput.c
 * Title: 5Vレギュレータを制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Regulator5VOutput.c
 * @brief 5Vレギュレータを制御する。
 */

#include "Regulator5VOutput.h"
#include "smpl_common.h"

//P46 
#define REGULATOR_5V_CONFIG (0x02 << 16)
#define REGULATOR_5V_VALUE	(0x40)

static OUTPUT_STATUS outputStatus = OUTPUT_STATUS_OFF;

void Regulator5VOutputInit(void)
{
	set_bit(PORT4->P4MOD1, REGULATOR_5V_CONFIG);
	Regulator5VOutputOff();
}

void Regulator5VOutputOn(void)
{
	outputStatus = OutputOnUInt32(&(PORT4->P4DO),REGULATOR_5V_VALUE);
}

void Regulator5VOutputOff(void)
{
	outputStatus = OutputOffUInt32(&(PORT4->P4DO),REGULATOR_5V_VALUE);
}

OUTPUT_STATUS Regulator5VOutputGetOutputStatus(void)
{
	return outputStatus;
}
