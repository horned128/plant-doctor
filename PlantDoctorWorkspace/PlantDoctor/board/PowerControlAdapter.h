#ifndef POWER_CONTROL_ADAPTER_H
#define POWER_CONTROL_ADAPTER_H

#include <stdbool.h>

bool PowerControlAdapter_Init(void);
bool PowerControlAdapter_IsPowerHeld(void);
void PowerControlAdapter_Shutdown(void);

#endif /* POWER_CONTROL_ADAPTER_H */
