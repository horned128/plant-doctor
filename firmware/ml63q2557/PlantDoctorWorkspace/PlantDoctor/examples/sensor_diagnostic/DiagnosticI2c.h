#ifndef DIAGNOSTIC_I2C_H
#define DIAGNOSTIC_I2C_H

#include <stdint.h>

typedef enum
{
	DIAGNOSTIC_I2C_OK = 0,
	DIAGNOSTIC_I2C_INVALID_ARGUMENT,
	DIAGNOSTIC_I2C_BUS_BUSY,
	DIAGNOSTIC_I2C_TIMEOUT,
	DIAGNOSTIC_I2C_NACK
} DIAGNOSTIC_I2C_STATUS;

DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Probe(uint8_t address7Bit);
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Write(
	uint8_t address7Bit,
	const uint8_t *data,
	uint16_t size);
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Read(
	uint8_t address7Bit,
	uint8_t *data,
	uint16_t size);
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_WriteRead(
	uint8_t address7Bit,
	const uint8_t *writeData,
	uint16_t writeSize,
	uint8_t *readData,
	uint16_t readSize);

#endif /* DIAGNOSTIC_I2C_H */
