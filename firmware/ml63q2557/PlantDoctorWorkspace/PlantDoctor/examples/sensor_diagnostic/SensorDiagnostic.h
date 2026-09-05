#ifndef SENSOR_DIAGNOSTIC_H
#define SENSOR_DIAGNOSTIC_H

#include <stdbool.h>
#include <stdint.h>

#include "DiagnosticSensors.h"

typedef struct
{
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

/* Inspect this symbol in the debugger even when the shared I2C LCD is unusable. */
extern volatile SENSOR_DIAGNOSTIC_SNAPSHOT g_sensorDiagnosticSnapshot;

void SensorDiagnostic_Init(void);
void SensorDiagnostic_RunOnce(void);

#endif /* SENSOR_DIAGNOSTIC_H */
