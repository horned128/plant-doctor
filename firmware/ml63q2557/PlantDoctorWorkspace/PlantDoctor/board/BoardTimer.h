/** =================================================================*
 * @file   BoardTimer.h
 * @brief  基板周期タイマAPI
 * ================================================================= */
#ifndef BOARD_TIMER_H
#define BOARD_TIMER_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

bool BoardTimer_Init(void);                                 /* BoardTimer_InitのAPI */
bool BoardTimer_Take10MsTick(void);                         /* BoardTimer_Take10MsTickのAPI */
bool BoardTimer_TakeOverflow(void);                         /* BoardTimer_TakeOverflowのAPI */
uint8_t BoardTimer_GetMaxPendingTicks(void);                /* 観測された最大未処理tick数 */
void BoardTimer_ClearMaxPendingTicks(void);                 /* 最大未処理tick数の記録をクリア */

#endif /* BOARD_TIMER_H */
