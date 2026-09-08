/*****************************************************************************
 * Copyright (C) 2024 ROHM Co., Ltd.
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
 *
 * Derived from the DT-EBML63Q2557 LCD sample. This project-local version
 * performs bounded polling so I2C work remains outside interrupt handlers.
 ******************************************************************************/

#include "LcdI2cf0.h"

#include <stdbool.h>
#include <stddef.h>

#include "PlantDoctorConfig.h"
#include "i2cf_common.h"
#include "mcu.h"
#include "rdwr_reg.h"
#include "wdt.h"

#define LCD_I2C_ACK                 (0U)
#define LCD_I2C_BUS_BUSY_MASK       (1UL << 5U)
#define LCD_I2C_TRANSMIT_MASK       (1UL << 4U)
#define LCD_I2C_MASTER_MASK         (1UL << 5U)
#define LCD_I2C_REPEAT_MASK         (1UL << 2U)
#define LCD_I2C_TX_NACK_MASK        (1UL << 3U)
#define LCD_I2C_CLOCK_HOLD_MASK     (1UL << 13U)
#define LCD_I2C_COMPLETE_MASK       (1UL << 7U)
#define LCD_I2C_CLEAR_MASK          ((1UL << 7U) | (1UL << 1U))
#define LCD_I2C_NACK_MASK           (1UL << 0U)

static bool LcdI2cf0_WaitForMask(uint32_t mask, bool set)
{
	uint32_t remaining = PLANT_DOCTOR_LCD_TIMEOUT_LOOPS;

	while (remaining > 0UL)
	{
		bool isSet = get_bit(I2CF0->I2F0SR, mask);
		if (isSet == set)
		{
			return true;
		}
		--remaining;
		if ((remaining & 0x3FFFUL) == 0UL)
		{
			wdt_clear();
		}
	}

	return false;
}

static bool LcdI2cf0_Stop(void)
{
	clear_bit(I2CF0->I2F0CTL, LCD_I2C_MASTER_MASK |
		LCD_I2C_CLOCK_HOLD_MASK |
		LCD_I2C_REPEAT_MASK |
		LCD_I2C_TX_NACK_MASK);
	return LcdI2cf0_WaitForMask(LCD_I2C_BUS_BUSY_MASK, false);
}

static LCD_I2C_STATUS LcdI2cf0_WaitByteComplete(void)
{
	bool nackDetected;

	if (!LcdI2cf0_WaitForMask(LCD_I2C_COMPLETE_MASK, true))
	{
		return LCD_I2C_STATUS_TRANSFER_TIMEOUT;
	}

	nackDetected = get_bit(I2CF0->I2F0SR, LCD_I2C_NACK_MASK) != LCD_I2C_ACK;
	clear_bit(I2CF0->I2F0SR, LCD_I2C_CLEAR_MASK);
	if (nackDetected)
	{
		return LCD_I2C_STATUS_NACK;
	}

	return LCD_I2C_STATUS_OK;
}

void LcdI2cf0_InitNormalMode(uint8_t mode, uint8_t rate)
{
	/* I2F0BC must be set before the module-enable bit according to the MCU manual. */
	clear_bit(I2CF0->I2F0CTL, (1UL << 7U));
	write_reg32(I2CF0->I2F0BC, rate);
	write_reg32(I2CF0->I2F0CTL, ((uint32_t)(mode & 0x03U) | (1UL << 7U)));
	clear_bit(I2CF0->I2F0MOD, (1UL << 0U));
	clear_bit(I2CF0->I2F0CTL, (1UL << 12U));
	clear_bit(I2CF0->I2F0CTL, (1UL << 11U));
	set_bit(I2CF0->I2F0CTL, (1UL << 9U));
	clear_bit(I2CF0->I2F0CTL, (1UL << 6U));
}

LCD_I2C_STATUS LcdI2cf0_Write(uint8_t slaveAddress, const uint8_t *data, uint16_t size)
{
	LCD_I2C_STATUS status;
	uint16_t index;

	if ((data == NULL) || (size == 0U))
	{
		return LCD_I2C_STATUS_INVALID_ARGUMENT;
	}

	if (!LcdI2cf0_WaitForMask(LCD_I2C_BUS_BUSY_MASK, false))
	{
		return LCD_I2C_STATUS_BUS_BUSY_TIMEOUT;
	}

	/* A preceding sensor receive can leave receiver-only state behind. */
	clear_bit(I2CF0->I2F0CTL, LCD_I2C_CLOCK_HOLD_MASK |
		LCD_I2C_REPEAT_MASK |
		LCD_I2C_TX_NACK_MASK);
	set_bit(I2CF0->I2F0CTL, LCD_I2C_TRANSMIT_MASK);
	write_reg32(I2CF0->I2F0DR, slaveAddress);
	set_bit(I2CF0->I2F0CTL, LCD_I2C_MASTER_MASK);

	status = LcdI2cf0_WaitByteComplete();
	if (status != LCD_I2C_STATUS_OK)
	{
		(void)LcdI2cf0_Stop();
		return status;
	}

	for (index = 0U; index < size; ++index)
	{
		write_reg32(I2CF0->I2F0DR, data[index]);
		status = LcdI2cf0_WaitByteComplete();
		if (status != LCD_I2C_STATUS_OK)
		{
			(void)LcdI2cf0_Stop();
			return status;
		}
	}

	if (!LcdI2cf0_Stop())
	{
		return LCD_I2C_STATUS_BUS_BUSY_TIMEOUT;
	}
	return LCD_I2C_STATUS_OK;
}
