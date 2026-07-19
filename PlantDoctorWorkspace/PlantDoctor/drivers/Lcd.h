/*****************************************************************************
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
 ******************************************************************************/

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#define LCD_DISPLAY_ON                    (1U)
#define LCD_CURSOR_OFF                    (0U)
#define LCD_CURSOR_BLINK_OFF              (0U)
#define LCD_START_OF_FIRST_LINE           (1U)
#define LCD_START_OF_SECOND_LINE          (17U)
#define LCD_END_OF_SECOND_LINE            (32U)
#define LCD_MOST_CHARACTERS_ON_A_LINE     (16U)

typedef enum
{
	LCD_STATUS_OK = 0,
	LCD_STATUS_INVALID_ARGUMENT,
	LCD_STATUS_BUS_BUSY_TIMEOUT,
	LCD_STATUS_TRANSFER_TIMEOUT,
	LCD_STATUS_NACK,
	LCD_STATUS_DELAY_ERROR
} LCD_STATUS;

void Lcd_PeripheralInit(void);
LCD_STATUS Lcd_Init(void);
LCD_STATUS Lcd_Draw(uint8_t position, const char *text);
LCD_STATUS Lcd_ClearDisplay(void);
LCD_STATUS Lcd_DisplayOnOff(uint8_t display, uint8_t cursor, uint8_t cursorBlink);
void Lcd_BacklightOn(void);
void Lcd_BacklightOff(void);

#endif /* LCD_H */
