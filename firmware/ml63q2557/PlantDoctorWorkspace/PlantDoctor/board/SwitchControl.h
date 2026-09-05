/** =================================================================*
 * @file   SwitchControl.h
 * @brief  スイッチ制御API
 * ================================================================= */
#ifndef SWITCH_CONTROL_H
#define SWITCH_CONTROL_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

#define SWITCH_CONTROL_PSW1                (1U << 0U)
#define SWITCH_CONTROL_PSW2                (1U << 1U)
#define SWITCH_CONTROL_PSW3                (1U << 2U)
#define SWITCH_CONTROL_PSW4                (1U << 3U)

bool SwitchControl_Init(void);                              /* SwitchControl_InitのAPI */
bool SwitchControl_Process10Ms(void);                       /* SwitchControl_Process10MsのAPI */
uint8_t SwitchControl_GetPressedMask(void);                 /* SwitchControl_GetPressedMaskのAPI */

#endif /* SWITCH_CONTROL_H */
