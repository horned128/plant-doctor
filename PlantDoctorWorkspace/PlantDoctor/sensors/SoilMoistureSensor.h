#ifndef SOIL_MOISTURE_SENSOR_H
#define SOIL_MOISTURE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

bool SoilMoistureSensor_Init(void);
bool SoilMoistureSensor_Read(uint16_t *rawValue);

#endif /* SOIL_MOISTURE_SENSOR_H */
