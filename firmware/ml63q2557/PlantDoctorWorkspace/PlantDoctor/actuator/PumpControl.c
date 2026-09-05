#include "PumpControl.h"

static bool s_isOn;

bool PumpControl_Init(void)
{
	s_isOn = false;
	return true;
}

PUMP_CONTROL_STATUS PumpControl_Request(bool on)
{
	(void)on;
	s_isOn = false;
	return PUMP_CONTROL_STATUS_NOT_IMPLEMENTED;
}

bool PumpControl_IsOn(void)
{
	return s_isOn;
}
