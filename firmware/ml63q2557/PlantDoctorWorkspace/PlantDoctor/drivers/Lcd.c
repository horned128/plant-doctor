/*****************************************************************************
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
 *
 * Derived from the DT-EBML63Q2557 LCD sample with bounded error handling.
 ******************************************************************************/

#include "Lcd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "LcdI2cf0.h"
#include "Output.h"
#include "TimeControl.h"
#include "i2cf_common.h"
#include "irq.h"
#include "mcu.h"
#include "rdwr_reg.h"
#include "smpl_common.h"

#define LCD_RESET_DISABLE              (1UL << 2U)
#define LCD_I2C_MODE                   (I2F_MOD_STD)
#define LCD_I2C_RATE                   (0x3CU)
#define LCD_CONTROL_COMMAND            (0x00U)
#define LCD_CONTROL_DATA               (0x40U)
#define LCD_SLAVE_ADDRESS              (0x7CU)
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

static LCD_STATUS Lcd_MapI2cStatus(LCD_I2C_STATUS status)
{
	switch (status)
	{
		case LCD_I2C_STATUS_OK:
			return LCD_STATUS_OK;
		case LCD_I2C_STATUS_BUS_BUSY_TIMEOUT:
			return LCD_STATUS_BUS_BUSY_TIMEOUT;
		case LCD_I2C_STATUS_TRANSFER_TIMEOUT:
			return LCD_STATUS_TRANSFER_TIMEOUT;
		case LCD_I2C_STATUS_NACK:
			return LCD_STATUS_NACK;
		case LCD_I2C_STATUS_INVALID_ARGUMENT:
		default:
			return LCD_STATUS_INVALID_ARGUMENT;
	}
}

static LCD_STATUS Lcd_Write(const uint8_t *data, uint16_t size)
{
	return Lcd_MapI2cStatus(LcdI2cf0_Write(LCD_SLAVE_ADDRESS, data, size));
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
			delayMs = 1U;
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

static bool Lcd_CreateTextPacket(const char *text, uint8_t *packet, uint16_t *packetSize)
{
	uint16_t length = 0U;

	if ((text == NULL) || (packet == NULL) || (packetSize == NULL))
	{
		return false;
	}

	while ((length <= LCD_MOST_CHARACTERS_ON_A_LINE) && (text[length] != '\0'))
	{
		++length;
	}
	if ((length == 0U) || (length > LCD_MOST_CHARACTERS_ON_A_LINE))
	{
		return false;
	}

	packet[0] = LCD_CONTROL_DATA;
	for (uint16_t index = 0U; index < length; ++index)
	{
		packet[index + 1U] = (uint8_t)text[index];
	}
	*packetSize = length + 1U;
	return true;
}

void Lcd_PeripheralInit(void)
{
	uint32_t interruptState = __get_PRIMASK();

	__disable_irq();
	irq_i2cf0_dis();
	smpl_enablePeripheral(I2CF0_PERI);
	set_reg32(PORT7->P7MOD0, (0x2BUL << 24U) | (0x02UL << 16U));
	set_reg32(PORT7->P7MOD1, (0x02UL << 8U) | (0x2BUL << 0U));
	set_bit(PORT7->P7DO, LCD_RESET_DISABLE);
	Lcd_BacklightOff();
	LcdI2cf0_InitNormalMode(LCD_I2C_MODE, LCD_I2C_RATE);
	irq_i2cf0_clearIRQ();
	if (interruptState == 0U)
	{
		__enable_irq();
	}
}

LCD_STATUS Lcd_Init(void)
{
	LCD_STATUS status;
	uint8_t index;

	TimeControlInit();
	status = Lcd_Delay(LCD_DELAY_RESET);
	for (index = 0U; (index < LCD_INIT_COMMAND_COUNT) && (status == LCD_STATUS_OK); ++index)
	{
		LCD_DELAY_KIND delayKind = LCD_DELAY_NORMAL;

		if (index == 5U)
		{
			delayKind = LCD_DELAY_POWER_STABLE;
		}
		else if (s_initCommands[index] == 0x01U)
		{
			delayKind = LCD_DELAY_CLEAR;
		}
		status = Lcd_WriteCommand(s_initCommands[index], delayKind);
	}
	return status;
}

LCD_STATUS Lcd_Draw(uint8_t position, const char *text)
{
	uint8_t address;
	uint8_t positionPacket[2] = {LCD_CONTROL_COMMAND, 0U};
	uint8_t textPacket[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
	uint16_t textPacketSize;
	LCD_STATUS status;

	if (!Lcd_CreatePosition(position, &address) ||
		!Lcd_CreateTextPacket(text, textPacket, &textPacketSize))
	{
		return LCD_STATUS_INVALID_ARGUMENT;
	}

	positionPacket[1] = address;
	status = Lcd_Write(positionPacket, 2U);
	if (status == LCD_STATUS_OK)
	{
		status = Lcd_Delay(LCD_DELAY_NORMAL);
	}
	if (status == LCD_STATUS_OK)
	{
		status = Lcd_Write(textPacket, textPacketSize);
	}
	if (status == LCD_STATUS_OK)
	{
		status = Lcd_Delay(LCD_DELAY_NORMAL);
	}
	return status;
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
