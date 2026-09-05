/** =================================================================*
 * @file   SoilMoistureSensor.h
 * @brief  土壌水分センサーAPI
 * ================================================================= */
#ifndef SOIL_MOISTURE_SENSOR_H
#define SOIL_MOISTURE_SENSOR_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

bool SoilMoistureSensor_Init(void);                         /* SoilMoistureSensor_InitのAPI */
bool SoilMoistureSensor_Read(uint16_t *rawValue);           /* SoilMoistureSensor_ReadのAPI */

#endif /* SOIL_MOISTURE_SENSOR_H */
