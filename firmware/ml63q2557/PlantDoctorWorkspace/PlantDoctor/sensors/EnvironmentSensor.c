#include "EnvironmentSensor.h"

bool EnvironmentSensor_Init(void)
{
	/* Temperature, humidity and light sensor models are not selected yet. */
	return true;
}

bool EnvironmentSensor_Read(ENVIRONMENT_SENSOR_SAMPLE *sample)
{
	(void)sample;
	return false;
}
