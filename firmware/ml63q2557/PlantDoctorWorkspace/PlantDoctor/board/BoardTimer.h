/** =================================================================*
 * @file   BoardTimer.h
 * @brief  基板周期タイマAPI
 * ================================================================= */
#ifndef BOARD_TIMER_H
#define BOARD_TIMER_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */

bool BoardTimer_Init(void);                                 /* BoardTimer_InitのAPI */
bool BoardTimer_Take10MsTick(void);                         /* BoardTimer_Take10MsTickのAPI */
bool BoardTimer_TakeOverflow(void);                         /* BoardTimer_TakeOverflowのAPI */

#endif /* BOARD_TIMER_H */
