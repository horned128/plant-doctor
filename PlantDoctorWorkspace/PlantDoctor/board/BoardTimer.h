#ifndef BOARD_TIMER_H
#define BOARD_TIMER_H

#include <stdbool.h>

bool BoardTimer_Init(void);
bool BoardTimer_Take10MsTick(void);
bool BoardTimer_TakeOverflow(void);

#endif /* BOARD_TIMER_H */
