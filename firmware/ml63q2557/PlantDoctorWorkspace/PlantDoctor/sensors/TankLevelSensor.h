/** =================================================================*
 * @file   TankLevelSensor.h
 * @brief  液面センサーAPI
 * ================================================================= */
#ifndef TANK_LEVEL_SENSOR_H
#define TANK_LEVEL_SENSOR_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */

bool TankLevelSensor_Init(void);                            /* 液面入力の初期化API */
bool TankLevelSensor_IsLiquidDetected(void);                /* 液面検出状態取得API */

#endif /* TANK_LEVEL_SENSOR_H */
