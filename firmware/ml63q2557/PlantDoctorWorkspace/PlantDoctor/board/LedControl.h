/** =================================================================*
 * @file   LedControl.h
 * @brief  LED制御API
 * ================================================================= */
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

typedef enum {
    LED_CONTROL_1 = 0,
    LED_CONTROL_2,
    LED_CONTROL_3,
    LED_CONTROL_COUNT
} LED_CONTROL_ID;

void LedControl_Init(void);                                 /* LedControl_InitのAPI */
void LedControl_Set(LED_CONTROL_ID id, bool on);            /* LedControl_SetのAPI */
void LedControl_Toggle(LED_CONTROL_ID id);                  /* LedControl_ToggleのAPI */
void LedControl_AllOff(void);                               /* LedControl_AllOffのAPI */

#endif /* LED_CONTROL_H */
