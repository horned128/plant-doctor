/** =================================================================*
 * @file   LeafTemperatureSensor.h
 * @brief  葉温センサーAPI
 * ================================================================= */
#ifndef LEAF_TEMPERATURE_SENSOR_H
#define LEAF_TEMPERATURE_SENSOR_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

bool LeafTemperatureSensor_Init(void);                      /* LeafTemperatureSensor_InitのAPI */
bool LeafTemperatureSensor_Read(int16_t *temperatureCentiC); /* LeafTemperatureSensor_ReadのAPI */

#endif /* LEAF_TEMPERATURE_SENSOR_H */
