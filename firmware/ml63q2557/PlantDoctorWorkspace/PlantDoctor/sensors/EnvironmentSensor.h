#ifndef ENVIRONMENT_SENSOR_H
#define ENVIRONMENT_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
	int16_t airTemperatureCentiC;
	uint16_t relativeHumidityCentiPercent;
	uint16_t illuminanceRaw;
} ENVIRONMENT_SENSOR_SAMPLE;

bool EnvironmentSensor_Init(void);
bool EnvironmentSensor_Read(ENVIRONMENT_SENSOR_SAMPLE *sample);

#endif /* ENVIRONMENT_SENSOR_H */
