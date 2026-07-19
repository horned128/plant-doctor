/*****************************************************************************
 * File: RelayOutput.c
 * Title: リレー出力を使う。
 * LastUpdated: 2025.06.02
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file RelayOutput.c
 * @brief リレー出力を使う。\n
 *		  回路図のISOOUT0をリレー0とする。\n
 *		  回路図のISOOUT1をリレー1とする。
 */

#include "RelayOutput.h"
#include "mcu.h"
#include "rdwr_reg.h"
#include "Output.h"

#define RELAY0_CONFIG	( (0x0A) << 16 )
#define RELAY1_CONFIG	( (0x0A) << 24 )
#define RELAY0_VALUE	(0x40)
#define RELAY1_VALUE	(0x80)

void RelayOutputInit(void)
{
	set_bit(PORT6->P6MOD1, RELAY0_CONFIG);
	set_bit(PORT5->P5MOD1, RELAY1_CONFIG);
	
	//プルアップで出力されているためオフにする。
	RelayOutputRelay0Off();
	RelayOutputRelay1Off();
}

void RelayOutputRelay0Off(void)
{
	OutputOnUInt32(&(PORT6->P6DO), (uint32_t)RELAY0_VALUE);
}

void RelayOutputRelay0On(void)
{
	OutputOffUInt32(&(PORT6->P6DO), (uint32_t)RELAY0_VALUE);
}

void RelayOutputRelay1Off(void)
{
	OutputOnUInt32(&(PORT5->P5DO), (uint32_t)RELAY1_VALUE);
}

void RelayOutputRelay1On(void)
{
	OutputOffUInt32(&(PORT5->P5DO), (uint32_t)RELAY1_VALUE);
}
