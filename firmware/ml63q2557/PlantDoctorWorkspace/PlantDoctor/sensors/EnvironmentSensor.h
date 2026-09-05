/** =================================================================*
 * @file   EnvironmentSensor.h
 * @brief  環境センサーAPI
 * ================================================================= */
#ifndef ENVIRONMENT_SENSOR_H
#define ENVIRONMENT_SENSOR_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

typedef struct {
    int16_t airTemperatureCentiC;
    uint16_t relativeHumidityCentiPercent;
    uint16_t illuminanceRaw;
} ENVIRONMENT_SENSOR_SAMPLE;

bool EnvironmentSensor_Init(void);                          /* EnvironmentSensor_InitのAPI */
bool EnvironmentSensor_Read(ENVIRONMENT_SENSOR_SAMPLE *sample); /* EnvironmentSensor_ReadのAPI */

#endif /* ENVIRONMENT_SENSOR_H */
