#include "SensorManager.h"

#include "EnvironmentSensor.h"
#include "LeafTemperatureSensor.h"
#include "SoilMoistureSensor.h"

static PLANT_SENSOR_SNAPSHOT s_latest;

bool SensorManager_Init(void)
{
	s_latest.soilMoistureRaw = 0U;
	s_latest.leafTemperatureCentiC = 0;
	s_latest.airTemperatureCentiC = 0;
	s_latest.relativeHumidityCentiPercent = 0U;
	s_latest.illuminanceRaw = 0U;
	s_latest.valid = false;

	return SoilMoistureSensor_Init() &&
		LeafTemperatureSensor_Init() &&
		EnvironmentSensor_Init();
}

void SensorManager_Process10Ms(void)
{
	/* Scheduling boundary for future non-blocking sensor drivers. */
}

bool SensorManager_GetLatest(PLANT_SENSOR_SNAPSHOT *snapshot)
{
	if ((snapshot == 0) || !s_latest.valid)
	{
		return false;
	}
	*snapshot = s_latest;
	return true;
}
