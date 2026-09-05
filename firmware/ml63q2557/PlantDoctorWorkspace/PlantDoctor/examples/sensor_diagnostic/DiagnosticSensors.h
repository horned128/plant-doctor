#ifndef DIAGNOSTIC_SENSORS_H
#define DIAGNOSTIC_SENSORS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	SENSOR_READING_OK = 0,
	SENSOR_READING_NACK,
	SENSOR_READING_BUS_BUSY,
	SENSOR_READING_TIMEOUT,
	SENSOR_READING_CRC,
	SENSOR_READING_INVALID_DATA
} SENSOR_READING_STATUS;

typedef struct
{
	SENSOR_READING_STATUS status;
	int16_t objectTemperatureCentiC;
	int16_t ambientTemperatureCentiC;
} SEN0206_READING;

typedef struct
{
	SENSOR_READING_STATUS status;
	int16_t temperatureCentiC;
	uint16_t humidityCentiPercent;
} SEN0385_READING;

typedef struct
{
	SENSOR_READING_STATUS status;
	uint32_t illuminanceCentiLux;
	uint16_t raw;
} SEN0228_READING;

typedef struct
{
	SENSOR_READING_STATUS status;
	uint16_t raw;
	uint16_t millivolts;
} SEN0193_READING;

void DiagnosticSensors_Init(void);
void DiagnosticSensors_ReadSen0206(SEN0206_READING *reading);
void DiagnosticSensors_ReadSen0385(SEN0385_READING *reading);
void DiagnosticSensors_ReadSen0228(SEN0228_READING *reading);
void DiagnosticSensors_ReadSen0193(SEN0193_READING *reading);
bool DiagnosticSensors_ReadSen0204(void);
const char *DiagnosticSensors_StatusText(SENSOR_READING_STATUS status);

#endif /* DIAGNOSTIC_SENSORS_H */
