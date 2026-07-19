/*****************************************************************************
 * File: UartInterrupt.c
 * Title: UARTの割り込み処理を担当する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file UartInterrupt.c
 * @brief UARTの割り込み処理を担当する。
 */

#include "mcu.h"
#include <stdio.h>
#include "UartInterrupt.h"

static UartInterruptFunc interruptFunc = NULL;
void UAF1_IRQHandler( void );

void UartInterruptSetFunc(UartInterruptFunc func)
{
	interruptFunc = func;
}

void UAF1_IRQHandler( void )
{
	if(interruptFunc == NULL) return;
	interruptFunc();
}
