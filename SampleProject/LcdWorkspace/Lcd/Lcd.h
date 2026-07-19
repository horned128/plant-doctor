/*****************************************************************************
 * File: Lcd.h
 * Title: Lcdを制御する。
 * LastUpdated: 2025.06.02
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Lcd.h
 * @brief Lcdを制御する。
 */

#ifndef LCD_H__
#define LCD_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief ディスプレイ表示
 */
#define LCD_DISPLAY_ON					(1)

/**
 * @brief カーソルOn
 */	
#define LCD_CURSOR_ON					(1)	

/**
 * @brief カーソルOff
 */	
#define LCD_CURSOR_OFF					(0)	

/**
 * @brief カーソル点滅なし
 */	
#define LCD_CURSOR_BLINK_OFF			(0)

/**
 * @brief Displayの一行目の先頭
 */
#define LCD_START_OF_FIRST_LINE			(1)

/**
 * @brief Displayの二行目の先頭
 */
#define LCD_START_OF_SECOND_LINE		(17)

/**
 * @brief Displayの末尾
 */
#define LCD_END_OF_SECOND_LINE			(32)

/**
 * @brief Displayの一行に表示できる最大文字数
 */
#define LCD_MOST_CHARACTERS_ON_A_LINE	(16)

/**
 * @brief Lcdを使用するために必要なCPUの設定を行う。
 */
void LcdPeripheralInit(void);

/**
 * @brief Lcdの初期化をする。
 */
void LcdInit(void);

/**
 * @brief Lcd上の指定した場所に文字を描画する。
 * 
 * @param position 描画場所 1行目: 1 ~ 16 2行目:17 ~ 32
 * @param str 描画する文字列　文字数: 1 ~ 16
 * @return bool 描画場所が範囲外、文字数が範囲外だとfalseを返す。
 */
bool LcdDraw(uint8_t position, const char* str);

/**
 * @brief 描画するLcdの場所を決定する。
 * 
 * @param position 描画場所　1行目: 1 ~ 16 2行目:17 ~ 32
 * @return bool 描画場所が範囲外だとfalseを返す。
 */
bool LcdSetPositionForDisplay(uint8_t position);

/**
 * @brief 文字を描画する。

 * @param str 描画する文字列 文字数: 1 ~ 16
 * @return bool 文字数が範囲外だとfalseを返す。
 */
bool LcdPutS(const char* str);

/**
 * @brief Lcdの表示をクリアする。
 */
void LcdClearDisplay(void);

/**
 * @brief Lcdの表示設定を行う。

 * @param display 文字を表示するかどうかを決定する。
 * @param cursor カーソルを表示するかどうかを決定する。
 * @param cursorBlink カーソルを点滅させて表示するかどうかを決定する。
 */
void LcdDisplayOnOff(uint8_t display, uint8_t cursor, uint8_t cursorBlink);

/**
 * @brief カーソルを右に1つ移動する。
 */
void LcdShiftCursorRight(void);

/**
 * @brief カーソルを左に1つ移動する。
 */
void LcdShiftCursorLeft(void);

/**
 * @brief Lcdのバックライトを点灯する。
 *
 * @note 点灯するには事前に5Vレギュレータを起動しておく。
 */
void LcdBacklightOn(void);

/**
 * @brief Lcdのバックライトを消灯する。
 */
void LcdBacklightOff(void);
#endif //LCD_H__
