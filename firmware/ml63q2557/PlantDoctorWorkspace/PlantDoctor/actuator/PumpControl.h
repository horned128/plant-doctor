/** =================================================================*
 * @file   PumpControl.h
 * @brief  ポンプ制御API
 * ================================================================= */
#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */

typedef enum {
    PUMP_CONTROL_STATUS_OK = 0,
    PUMP_CONTROL_STATUS_NOT_IMPLEMENTED
} PUMP_CONTROL_STATUS;

bool PumpControl_Init(void);                                /* PumpControl_InitのAPI */
PUMP_CONTROL_STATUS PumpControl_Request(bool on);           /* PumpControl_RequestのAPI */
bool PumpControl_IsOn(void);                                /* PumpControl_IsOnのAPI */

#endif /* PUMP_CONTROL_H */
