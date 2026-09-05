/** =================================================================*
 * @file   SensorDiagnostic.h
 * @brief  センサー診断アプリケーションAPI
 * ================================================================= */
#ifndef SENSOR_DIAGNOSTIC_H
#define SENSOR_DIAGNOSTIC_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "DiagnosticSensors.h"                              /* DiagnosticSensorsのAPIと型定義 */

typedef struct {
    SEN0206_READING sen0206;
    SEN0385_READING sen0385;
    SEN0228_READING sen0228;
    SEN0193_READING sen0193;
    bool sen0204LiquidDetected;
    bool lcdReady;
    uint8_t currentPage;
    uint8_t lastLcdStatus;
    uint32_t lcdFailureCount;
    uint32_t lcdRecoveryAttemptCount;
    uint32_t lcdRecoverySuccessCount;
    uint32_t refreshCount;
} SENSOR_DIAGNOSTIC_SNAPSHOT;

/* 共用I2C LCDが使用できない場合もデバッガでこの値を確認できる。 */
extern volatile SENSOR_DIAGNOSTIC_SNAPSHOT g_sensorDiagnosticSnapshot;

void SensorDiagnostic_Init(void);                           /* SensorDiagnostic_InitのAPI */
void SensorDiagnostic_RunOnce(void);                        /* SensorDiagnostic_RunOnceのAPI */

#endif /* SENSOR_DIAGNOSTIC_H */
