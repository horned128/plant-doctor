#ifndef LEAF_TEMPERATURE_SENSOR_H
#define LEAF_TEMPERATURE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

bool LeafTemperatureSensor_Init(void);
bool LeafTemperatureSensor_Read(int16_t *temperatureCentiC);

#endif /* LEAF_TEMPERATURE_SENSOR_H */
