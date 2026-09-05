/** =================================================================*
 * @file   PowerControlAdapter.h
 * @brief  電源制御アダプタAPI
 * ================================================================= */
#ifndef POWER_CONTROL_ADAPTER_H
#define POWER_CONTROL_ADAPTER_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */

bool PowerControlAdapter_Init(void);                        /* PowerControlAdapter_InitのAPI */
bool PowerControlAdapter_IsPowerHeld(void);                 /* PowerControlAdapter_IsPowerHeldのAPI */
void PowerControlAdapter_Shutdown(void);                    /* PowerControlAdapter_ShutdownのAPI */

#endif /* POWER_CONTROL_ADAPTER_H */
