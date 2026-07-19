#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	uint16_t soilMoistureRaw;
	int16_t leafTemperatureCentiC;
	int16_t airTemperatureCentiC;
	uint16_t relativeHumidityCentiPercent;
	uint16_t illuminanceRaw;
	bool valid;
} PLANT_SENSOR_SNAPSHOT;

bool SensorManager_Init(void);
void SensorManager_Process10Ms(void);
bool SensorManager_GetLatest(PLANT_SENSOR_SNAPSHOT *snapshot);

#endif /* SENSOR_MANAGER_H */
