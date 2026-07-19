/*****************************************************************************
 * File: Output.c
 * Title: 汎用出力を使う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Output.c
 * @brief 汎用出力を使う。
 */

#include "Output.h"
#include <stdio.h>
#include "rdwr_reg.h"

OUTPUT_STATUS OutputOnUInt8(volatile void* dst, uint8_t value)
{
	volatile uint8_t* output = (volatile uint8_t *)dst;
	if(output == NULL) return OUTPUT_STATUS_NULL;
	*output |= value;
	return OUTPUT_STATUS_ON;
}

OUTPUT_STATUS OutputOffUInt8(volatile void* dst, uint8_t value)
{
	volatile uint8_t* output = (volatile uint8_t *)dst;
	if(output == NULL) return OUTPUT_STATUS_NULL;
	*output &= ~value;
	return OUTPUT_STATUS_OFF;
}


OUTPUT_STATUS OutputOnUInt16(volatile void* dst, uint16_t value)
{
	volatile uint16_t* output = (volatile uint16_t *)dst;
	if(output == NULL) return OUTPUT_STATUS_NULL;
	*output |= value;
	return OUTPUT_STATUS_ON;
}

OUTPUT_STATUS OutputOffUInt16(volatile void* dst, uint16_t value)
{
	volatile uint16_t* output = (volatile uint16_t *)dst;
	if(output == NULL) return OUTPUT_STATUS_NULL;
	*output &= ~value;
	return OUTPUT_STATUS_OFF;
}


OUTPUT_STATUS OutputOnUInt32(volatile void* dst, uint32_t value)
{
	volatile uint32_t* output = (volatile uint32_t *)dst;
	if(output == NULL) return OUTPUT_STATUS_NULL;
	*output |= value;
	return OUTPUT_STATUS_ON;
}

OUTPUT_STATUS OutputOffUInt32(volatile void* dst, uint32_t value)
{
	volatile uint32_t* output = (volatile uint32_t *)dst;
	if(output == NULL) return OUTPUT_STATUS_NULL;
	*output &= ~value;
	return OUTPUT_STATUS_OFF;
}
