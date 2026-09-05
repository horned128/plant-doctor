#include "SoilMoistureSensor.h"

bool SoilMoistureSensor_Init(void)
{
	/* Sensor type and ADC channel are intentionally not selected yet. */
	return true;
}

bool SoilMoistureSensor_Read(uint16_t *rawValue)
{
	(void)rawValue;
	return false;
}
