/** =================================================================*
 * @file   I2cBus.h
 * @brief  I2CF0共通通信API
 * ================================================================= */
#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */

typedef enum {
    I2C_BUS_OK = 0,
    I2C_BUS_INVALID_ARGUMENT,
    I2C_BUS_BUSY,
    I2C_BUS_TIMEOUT,
    I2C_BUS_NACK
} I2C_BUS_STATUS;

bool I2cBus_Init(void);                                     /* I2Cバス初期化API */
bool I2cBus_Recover(void);                                  /* I2Cバス復旧API */
I2C_BUS_STATUS I2cBus_Write(uint8_t address7Bit, const uint8_t *data, uint16_t size); /* I2C書き込みAPI */
I2C_BUS_STATUS I2cBus_Read(uint8_t address7Bit, uint8_t *data, uint16_t size); /* I2C読み出しAPI */
I2C_BUS_STATUS I2cBus_WriteRead(uint8_t address7Bit, const uint8_t *writeData,
    uint16_t writeSize, uint8_t *readData, uint16_t readSize); /* I2C連続転送API */

#endif /* I2C_BUS_H */
