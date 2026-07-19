/*****************************************************************************
 * Copyright (C) 2024 ROHM Co., Ltd.
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
 ******************************************************************************/

#ifndef LCD_I2CF0_H
#define LCD_I2CF0_H

#include <stdint.h>

typedef enum
{
	LCD_I2C_STATUS_OK = 0,
	LCD_I2C_STATUS_INVALID_ARGUMENT,
	LCD_I2C_STATUS_BUS_BUSY_TIMEOUT,
	LCD_I2C_STATUS_TRANSFER_TIMEOUT,
	LCD_I2C_STATUS_NACK
} LCD_I2C_STATUS;

void LcdI2cf0_InitNormalMode(uint8_t mode, uint8_t rate);
LCD_I2C_STATUS LcdI2cf0_Write(uint8_t slaveAddress, const uint8_t *data, uint16_t size);

#endif /* LCD_I2CF0_H */
