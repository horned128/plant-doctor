/*****************************************************************************
 * File: UartInterrupt.h
 * Title: UARTの割り込み処理を担当する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file UartInterrupt.h
 * @brief UARTの割り込み処理を担当する。
 */

#ifndef UART_INTERRUPT_H__
#define UART_INTERRUPT_H__
/** 
 * @brief UART割込みで使用する関数型
 */
typedef void (*UartInterruptFunc)(void);

/** 
 * @brief UART割込みで使用する関数を設定する。
 *
 * @param func 使用する関数
 */
void UartInterruptSetFunc(UartInterruptFunc func);
#endif //UART_INTERRUPT_H__
