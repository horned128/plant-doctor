/** =================================================================*
 * @file   DiagnosticI2c.h
 * @brief  診断用I2C通信API
 * ================================================================= */
#ifndef DIAGNOSTIC_I2C_H
#define DIAGNOSTIC_I2C_H

#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

typedef enum {
    DIAGNOSTIC_I2C_OK = 0,
    DIAGNOSTIC_I2C_INVALID_ARGUMENT,
    DIAGNOSTIC_I2C_BUS_BUSY,
    DIAGNOSTIC_I2C_TIMEOUT,
    DIAGNOSTIC_I2C_NACK
} DIAGNOSTIC_I2C_STATUS;

DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Probe(uint8_t address7Bit); /* DiagnosticI2c_ProbeのAPI */
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Write(
    uint8_t address7Bit,
    const uint8_t *data,
    uint16_t size);                                         /* DiagnosticI2c_WriteのAPI */
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_Read(
    uint8_t address7Bit,
    uint8_t *data,
    uint16_t size);                                         /* DiagnosticI2c_ReadのAPI */
DIAGNOSTIC_I2C_STATUS DiagnosticI2c_WriteRead(
    uint8_t address7Bit,
    const uint8_t *writeData,
    uint16_t writeSize,
    uint8_t *readData,
    uint16_t readSize);                                     /* DiagnosticI2c_WriteReadのAPI */

#endif /* DIAGNOSTIC_I2C_H */
