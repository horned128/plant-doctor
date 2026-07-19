#include "LcdUi.h"

#include "Lcd.h"
#include "SwitchControl.h"

static bool s_ready;

static bool LcdUi_WriteLine(uint8_t position, const char *text)
{
	char line[LCD_MOST_CHARACTERS_ON_A_LINE + 1U];
	uint8_t index = 0U;

	while (index < LCD_MOST_CHARACTERS_ON_A_LINE)
	{
		line[index] = ' ';
		++index;
	}
	line[LCD_MOST_CHARACTERS_ON_A_LINE] = '\0';

	index = 0U;
	while ((index < LCD_MOST_CHARACTERS_ON_A_LINE) && (text[index] != '\0'))
	{
		line[index] = text[index];
		++index;
	}

	return (Lcd_Draw(position, line) == LCD_STATUS_OK);
}

bool LcdUi_Init(void)
{
	LCD_STATUS status;

	s_ready = false;
	Lcd_PeripheralInit();
	status = Lcd_Init();
	if (status == LCD_STATUS_OK)
	{
		status = Lcd_DisplayOnOff(LCD_DISPLAY_ON, LCD_CURSOR_OFF, LCD_CURSOR_BLINK_OFF);
	}
	if (status == LCD_STATUS_OK)
	{
		Lcd_BacklightOn();
		s_ready = true;
	}
	return s_ready;
}

bool LcdUi_IsReady(void)
{
	return s_ready;
}

bool LcdUi_ShowBoardTest(void)
{
	return s_ready &&
		LcdUi_WriteLine(LCD_START_OF_FIRST_LINE, "PLANT DOCTOR") &&
		LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, "BOARD TEST");
}

bool LcdUi_ShowSwitch(uint8_t pressedMask)
{
	const char *message = "BOARD TEST";

	if ((pressedMask & SWITCH_CONTROL_PSW1) != 0U)
	{
		message = "SW1 PRESSED";
	}
	else if ((pressedMask & SWITCH_CONTROL_PSW2) != 0U)
	{
		message = "SW2 PRESSED";
	}
	else if ((pressedMask & SWITCH_CONTROL_PSW3) != 0U)
	{
		message = "SW3 PRESSED";
	}
	else if ((pressedMask & SWITCH_CONTROL_PSW4) != 0U)
	{
		message = "SW4 PRESSED";
	}

	return s_ready && LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, message);
}

bool LcdUi_ShowError(PLANT_DOCTOR_ERROR error)
{
	const char *message;

	switch (error)
	{
		case PLANT_DOCTOR_ERROR_POWER:
			message = "ERROR POWER";
			break;
		case PLANT_DOCTOR_ERROR_TIMER:
			message = "ERROR TIMER";
			break;
		case PLANT_DOCTOR_ERROR_SWITCH:
			message = "ERROR SWITCH";
			break;
		case PLANT_DOCTOR_ERROR_LCD_INIT:
		case PLANT_DOCTOR_ERROR_LCD_IO:
			message = "ERROR LCD";
			break;
		case PLANT_DOCTOR_ERROR_TICK_OVERFLOW:
			message = "ERROR TIMING";
			break;
		case PLANT_DOCTOR_ERROR_SENSOR_INTERFACE:
			message = "ERROR SENSOR";
			break;
		case PLANT_DOCTOR_ERROR_STORAGE_INTERFACE:
			message = "ERROR STORAGE";
			break;
		case PLANT_DOCTOR_ERROR_NONE:
		default:
			message = "ERROR UNKNOWN";
			break;
	}

	return s_ready &&
		LcdUi_WriteLine(LCD_START_OF_FIRST_LINE, "PLANT DOCTOR") &&
		LcdUi_WriteLine(LCD_START_OF_SECOND_LINE, message);
}
