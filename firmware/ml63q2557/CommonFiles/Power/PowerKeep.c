/*****************************************************************************
 * File: PowerKeep.c
 * Title: 電圧保持(POWER_KEEP)を制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PowerKeep.c
 * @brief 電圧保持(POWER_KEEP)を制御する。
 */

#include "PowerKeep.h"
#include "Output.h"
#include "smpl_common.h"

#define POWER_KEEP_VALUE	(0x20)
#define POWER_KEEP_CONFIG	(0x02U << 8U)

void PowerKeepInit(void)
{
	set_reg32(PORT4->P4MOD1,(POWER_KEEP_CONFIG));
	PowerKeepOff();
}

void PowerKeepOff(void)
{
	OutputOffUInt32(&(PORT4->P4DO),POWER_KEEP_VALUE);
}

void PowerKeepOn(void)
{
	OutputOnUInt32(&(PORT4->P4DO),POWER_KEEP_VALUE);
}
