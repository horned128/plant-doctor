#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <stdbool.h>

typedef enum
{
	PUMP_CONTROL_STATUS_OK = 0,
	PUMP_CONTROL_STATUS_NOT_IMPLEMENTED
} PUMP_CONTROL_STATUS;

bool PumpControl_Init(void);
PUMP_CONTROL_STATUS PumpControl_Request(bool on);
bool PumpControl_IsOn(void);

#endif /* PUMP_CONTROL_H */
