/** =================================================================*
 * @file   ConsoleUart.h
 * @brief  UARTF1ハードウェア連携API (P2-4)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef CONSOLE_UART_H
#define CONSOLE_UART_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include "Console.h"                        /* コンソールサービス定義 */

#ifdef __cplusplus
extern "C" {
#endif

bool ConsoleUart_Init(const CONSOLE_SERVICES *services);
void ConsoleUart_Process10Ms(void);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_UART_H */
