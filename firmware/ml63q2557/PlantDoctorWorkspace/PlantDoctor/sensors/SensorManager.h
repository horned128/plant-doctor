/** =================================================================*
 * @file   SensorManager.h
 * @brief  センサー管理API
 * ================================================================= */
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

typedef struct {
    uint16_t soilMoistureRaw;
    int16_t leafTemperatureCentiC;
    int16_t airTemperatureCentiC;
    uint16_t relativeHumidityCentiPercent;
    uint32_t barometricPressurePa;
    uint32_t illuminanceCentiLux;
    uint16_t illuminanceRaw;
    bool soilMoistureValid;
    bool leafTemperatureValid;
    bool airTemperatureValid;
    bool barometricPressureValid;
    bool illuminanceValid;
    bool tankLiquidDetected;
    bool valid;
} PLANT_SENSOR_SNAPSHOT;

bool SensorManager_Init(void);                              /* SensorManager_InitのAPI */
void SensorManager_Process10Ms(void);                       /* SensorManager_Process10MsのAPI */
bool SensorManager_GetLatest(PLANT_SENSOR_SNAPSHOT *snapshot); /* SensorManager_GetLatestのAPI */

#endif /* SENSOR_MANAGER_H */
