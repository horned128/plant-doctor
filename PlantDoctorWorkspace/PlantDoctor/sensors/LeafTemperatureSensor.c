#include "LeafTemperatureSensor.h"

bool LeafTemperatureSensor_Init(void)
{
	/* The infrared sensor and bus assignment are not selected yet. */
	return true;
}

bool LeafTemperatureSensor_Read(int16_t *temperatureCentiC)
{
	(void)temperatureCentiC;
	return false;
}
