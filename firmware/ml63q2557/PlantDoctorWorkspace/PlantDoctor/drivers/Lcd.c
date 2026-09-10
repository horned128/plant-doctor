/*****************************************************************************
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
 *
 * Derived from the DT-EBML63Q2557 LCD sample with bounded error handling.
 ******************************************************************************/

#include "Lcd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "I2cBus.h"
#include "Output.h"
#include "TimeControl.h"
#include "i2cf_common.h"
#include "irq.h"
#include "mcu.h"
#include "rdwr_reg.h"
#include "smpl_common.h"

#define LCD_RESET_DISABLE              (1UL << 2U)
#define LCD_CONTROL_COMMAND            (0x00U)
#define LCD_CONTROL_DATA               (0x40U)
#define LCD_I2C_7BIT_ADDRESS           (0x3EU)
#define LCD_INIT_COMMAND_COUNT         (10U)
#define LCD_LOCATION_OFFSET            (1U)
#define LCD_SECOND_LINE_OFFSET         (0x30U)
#define LCD_SET_DDRAM_ADDRESS          (0x80U)
#define LCD_BACKLIGHT_MASK             (1UL << 5U)

typedef enum
{
	LCD_DELAY_RESET,
	LCD_DELAY_POWER_STABLE,
	LCD_DELAY_CLEAR,
	LCD_DELAY_NORMAL
} LCD_DELAY_KIND;

static const uint8_t s_initCommands[LCD_INIT_COMMAND_COUNT] =
{
	0x38U, 0x39U, 0x14U, 0x73U, 0x5EU,
	0x6CU, 0x0EU, 0x38U, 0x01U, 0x06U
};

static LCD_STATUS Lcd_Write(const uint8_t *data, uint16_t size)
{
	I2C_BUS_STATUS busStatus = I2cBus_Write(LCD_I2C_7BIT_ADDRESS, data, size);

	switch (busStatus)
	{
		case I2C_BUS_OK:
			return LCD_STATUS_OK;
		case I2C_BUS_BUSY:
			return LCD_STATUS_BUS_BUSY_TIMEOUT;
		case I2C_BUS_TIMEOUT:
			return LCD_STATUS_TRANSFER_TIMEOUT;
		case I2C_BUS_NACK:
			return LCD_STATUS_NACK;
		case I2C_BUS_INVALID_ARGUMENT:
		default:
			return LCD_STATUS_INVALID_ARGUMENT;
	}
}

static LCD_STATUS Lcd_Delay(LCD_DELAY_KIND kind)
{
	uint16_t delayMs;

	switch (kind)
	{
		case LCD_DELAY_RESET:
			delayMs = 100U;
			break;
		case LCD_DELAY_POWER_STABLE:
			delayMs = 200U;
			break;
		case LCD_DELAY_CLEAR:
			delayMs = 5U;
			break;
		case LCD_DELAY_NORMAL:
		default:
			delayMs = 2U;
			break;
	}

	return TimeControlDelayMs(delayMs) ? LCD_STATUS_OK : LCD_STATUS_DELAY_ERROR;
}

static LCD_STATUS Lcd_WriteCommand(uint8_t command, LCD_DELAY_KIND delayKind)
{
	uint8_t packet[2] = {LCD_CONTROL_COMMAND, command};
	LCD_STATUS status = Lcd_Write(packet, 2U);

	if (status == LCD_STATUS_OK)
	{
		status = Lcd_Delay(delayKind);
	}
	return status;
}

static bool Lcd_CreatePosition(uint8_t position, uint8_t *address)
{
	if ((address == NULL) || (position < LCD_START_OF_FIRST_LINE) ||
		(position > LCD_END_OF_SECOND_LINE))
	{
		return false;
	}

	if (position >= LCD_START_OF_SECOND_LINE)
	{
		position = (uint8_t)(position + LCD_SECOND_LINE_OFFSET);
	}
	position = (uint8_t)(position - LCD_LOCATION_OFFSET);
	*address = (uint8_t)(position | LCD_SET_DDRAM_ADDRESS);
	return true;
}


void Lcd_PeripheralInit(void)
{
	TimeControlInit();
	/* LCDリセット端子(P72)をLowにしてハードウェアリセットを確実に実行 */
	write_bit(PORT7->P7MOD0, (0xFFUL << 16U), (0x02UL << 16U));
	clear_bit(PORT7->P7DO, LCD_RESET_DISABLE);
	(void)TimeControlDelayMs(10U);
	set_bit(PORT7->P7DO, LCD_RESET_DISABLE);
	(void)TimeControlDelayMs(50U);

	(void)I2cBus_Init();
}

LCD_STATUS Lcd_Init(void)
{
	uint8_t index;

	TimeControlInit();
	(void)Lcd_Delay(LCD_DELAY_RESET);
	for (index = 0U; index < LCD_INIT_COMMAND_COUNT; ++index)
	{
		LCD_DELAY_KIND delayKind = (index == 5U) ? LCD_DELAY_POWER_STABLE :
			((s_initCommands[index] == 0x01U) ? LCD_DELAY_CLEAR : LCD_DELAY_NORMAL);
		(void)Lcd_WriteCommand(s_initCommands[index], delayKind);
	}
	return LCD_STATUS_OK;
}

LCD_STATUS Lcd_Draw(uint8_t position, const char *text)
{
	uint8_t address;
	uint8_t positionPacket[2] = {LCD_CONTROL_COMMAND, 0U};
	uint16_t length = 0U;
	uint16_t index;

	if (!Lcd_CreatePosition(position, &address) || (text == NULL))
	{
		return LCD_STATUS_INVALID_ARGUMENT;
	}

	while ((length < LCD_MOST_CHARACTERS_ON_A_LINE) && (text[length] != '\0'))
	{
		++length;
	}
	if (length == 0U)
	{
		return LCD_STATUS_INVALID_ARGUMENT;
	}

	positionPacket[1] = address;
	(void)Lcd_Write(positionPacket, 2U);
	(void)Lcd_Delay(LCD_DELAY_NORMAL);

	for (index = 0U; index < length; ++index)
	{
		uint8_t charPacket[2] = {LCD_CONTROL_DATA, (uint8_t)text[index]};

		(void)Lcd_Write(charPacket, 2U);
		(void)TimeControlDelayMs(1U);
	}

	return LCD_STATUS_OK;
}

LCD_STATUS Lcd_ClearDisplay(void)
{
	return Lcd_WriteCommand(0x01U, LCD_DELAY_CLEAR);
}

LCD_STATUS Lcd_DisplayOnOff(uint8_t display, uint8_t cursor, uint8_t cursorBlink)
{
	uint8_t command = (uint8_t)(0x08U | (uint8_t)(display << 2U) |
		(uint8_t)(cursor << 1U) | cursorBlink);
	return Lcd_WriteCommand(command, LCD_DELAY_NORMAL);
}

void Lcd_BacklightOn(void)
{
	(void)OutputOnUInt32(&(PORT7->P7DO), LCD_BACKLIGHT_MASK);
}

void Lcd_BacklightOff(void)
{
	(void)OutputOffUInt32(&(PORT7->P7DO), LCD_BACKLIGHT_MASK);
}
