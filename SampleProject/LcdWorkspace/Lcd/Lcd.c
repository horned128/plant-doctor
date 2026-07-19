/*****************************************************************************
 * File: Lcd.c
 * Title: Lcdを制御する。
 * LastUpdated: 2025.06.02
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Lcd.c
 * @brief Lcdを制御する。
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "mcu.h"
#include "irq.h"
#include "wdt.h"
#include "LcdI2cf0.h"
#include "Lcd.h"
#include "TimeControl.h"
#include "smpl_common.h"
#include "Output.h"

/** LCDIO設定 */
#define LCD_RESET_DISABLE				(1 << 2)
/** LCD設定	*/
#define I2F_PARAM_MODE					(I2F_MOD_FST)
#define I2F_TRANS_RATE					(0x0F)

/** LCD最終書き込みデータ */
#define CONB_DATA_END					(0x40)
/** LCD最終書き込みコマンド */
#define CONB_COMMAND_END				(0x00)
/** LCD用コマンドサイズ */
#define COMMAND_SIZE					(1)
/** LCDスレーブアドレス */
#define LCD_SLAVE_ADDRESS				(0x7C)
/** I2Cアドレスサイズ	*/
#define I2C_ADDRESS_SIZE				(0)

/** LCD用初期化送信バッファサイズ */
#define COMMAND_BUFFER_SIZE				(20)
/** LCDのリセット処理*/
#define COMMAND_BUFFER_POWER_STABLE		(10)
/** LCDの表示場所オフセット */
#define LCD_LOCATION_OFFSET				(1)
/** LCDの2行目オフセット */
#define LCD_SECOND_LINE_OFFSET			(0x30)
/** LCD1行の最大文字数 */
#define LCD_DDRAM_SIZE_FOR_ONE_LINE		(LCD_MOST_CHARACTERS_ON_A_LINE)
/** LCD用バッファ最大サイズ */
#define MAX_OF_TXBUFFER_PUTS			(COMMAND_SIZE + LCD_DDRAM_SIZE_FOR_ONE_LINE + 1)
/** LCD用描画用オフセット */
#define SET_DDRAM_ADDRESS				(0x80)
/** LCDバックライト */
#define BACKLIGHT_VALUE					(0x20)

/** LCDに描画できる最大数 */
#define DRAW_MIN						(1)
#define DRAW_MAX						(LCD_DDRAM_SIZE_FOR_ONE_LINE)

/**
 * @brief Delay、Timeout用の列挙体
 */
typedef enum
{
	TIME_CONTROL_1MS = 1,
	TIME_CONTROL_5MS = 5,
	TIME_CONTROL_10MS = 10,
	TIME_CONTROL_WAIT_RESET = 100,
	TIME_CONTROL_WAIT_POWER_STABLE = 200,
}TIME_CONTROL;

/**
 * @brief LCDのコマンド用の列挙体
 */
typedef enum
{
	LCD_COMMAND_LCD_RESET,
	LCD_COMMAND_POWER_STABLE,
	LCD_COMMAND_CLEAR_DISPLAY,
	LCD_COMMAND_RETURN_HOME,
	LCD_COMMAND_ENTRY_MODE_SET,
	LCD_COMMAND_DISPLAY_ON_OFF,
	LCD_COMMAND_FUNCTION_SET,
	LCD_COMMAND_SET_DDRAM_ADDRESS,
	LCD_COMMAND_SET_CGRAM,
	LCD_COMMAND_WRITE_TO_DATA_RAM,
	LCD_COMMAND_CURSOR_OR_DISPLAY_SHIFT,
	LCD_COMMAND_INTERNAL_OSC_FREQUENCY,
	LCD_COMMAND_SET_ICON_ADDRESS,
	LCD_COMMAND_POWER_ICON_CONTROL_CONTRAST_SET,
	LCD_COMMAND_FOLLOWER_CONTROL,
	LCD_COMMAND_CONTRAST_SET
}LCD_COMMAND;

/**
 * @brief LCDの初期化用コマンドの列挙体
 */
typedef enum
{
	INITIALIZE_FIRST_FUNCTION_SET = 0x38,
	INITIALIZE_SECOND_FUNCTION_SET = 0x39,
	INITIALIZE_INTERNAL_OSC_FREUENCY = 0x14,
	INITIALIZE_CONTRAST_SET = 0x73,
	INITIALIZE_POWER_ICON_CONTROL_CONTRAST_SET = 0x5E,
	INITIALIZE_FOLLOWER_CONTROL = 0x6C,
	INITIALIZE_DISPLAY_ON_OFF = 0x0E,
	INITIALIZE_THIRD_FUNCTION_SET = 0x38,
	INITIALIZE_CLEAR_DISPLAY = 0x01,
	INITIALIZE_ENTRY_MODE_SET = 0x06,
}INITIALIZE;

volatile static bool i2cf0TransferEndFlg = false;
volatile static bool i2cf0NackFlg = false;


/**
 * @brief 表示場所を計算する。
 *
 * @param givenPosition 入力された場所
 * @param calculatedPosition 計算後の場所
 * @bool 計算が成功したならtrueを返す。
 */
static bool createPosition(uint8_t givenPosition, uint8_t* calculatedPositon);

/**
 * @brief Lcdに文字を表示するための文字列を作成する。
 *
 * @param str 元の文字列
 * @param array 送信用文字列 
 * @param trasmitDataSize 送信用文字列のデータサイズ
 * @bool 成功したらtrueを返す。
 */
static bool createStringData(const char* str, uint8_t* array, uint8_t* trasmitDataSize);

/**
 * @brief I2c通信時にnackが発生したかを確認する。
 *
 * @bool nackが発生したならtrueを返す。
 */
static bool isNackOccurred(void);

/**
 * @brief LCDに対してのコマンド処理全体でかかる時間を待つ。
 *
 * @param cmd 送るコマンドの種類
 */
static void waitUnitilEntireProcess(LCD_COMMAND cmd);

/**
 * @brief I2c通信が完了するまで待つ。
 */
static void lcdWaitI2cf0TransferEnd(void);

/**
 * @brief I2c通信完了時のコールバック
 *
 * @param size データ数
 * @param errStat エラーステータス
 */
static void lcdTransferEnd( uint32_t size, uint8_t errStat );

/**
 * @brief LCDがコマンドを処理する時間待つ。
 *
 * @param cmd 送るコマンドの種類
 */
static void waitExecuteLcdCommand(LCD_COMMAND cmd);




void LcdPeripheralInit(void)
{
	__disable_irq();
	irq_i2cf0_dis();
	smpl_enablePeripheral(I2CF0_PERI);
	
	//P72 リセット、P73 SCLF0   
	set_reg32(PORT7->P7MOD0,  (0x2b << 24) | (0x02 << 16) );
	//P74 SDAF0 P75 バックライト 
	set_reg32(PORT7->P7MOD1,  (0x02 << 8) | (0x2b << 0) );
	PORT7->P7DO |= LCD_RESET_DISABLE;
	LcdBacklightOff();
	
	LcdI2cf0InitNormalMode( I2F_PARAM_MODE, I2F_TRANS_RATE );
	i2cf0TransferEndFlg = false;
	i2cf0NackFlg = false;
	irq_i2cf0_setLevel(0);
	irq_i2cf0_clearIRQ();
	irq_i2cf0_ena();
	__enable_irq();
}

void LcdInit(void)
{
	uint8_t command[COMMAND_BUFFER_SIZE] = 
	{ 
		CONB_COMMAND_END , INITIALIZE_FIRST_FUNCTION_SET ,
		CONB_COMMAND_END , INITIALIZE_SECOND_FUNCTION_SET ,
		CONB_COMMAND_END , INITIALIZE_INTERNAL_OSC_FREUENCY ,
		CONB_COMMAND_END , INITIALIZE_CONTRAST_SET ,
		CONB_COMMAND_END , INITIALIZE_POWER_ICON_CONTROL_CONTRAST_SET ,
		CONB_COMMAND_END , INITIALIZE_FOLLOWER_CONTROL ,
		CONB_COMMAND_END , INITIALIZE_DISPLAY_ON_OFF ,
		CONB_COMMAND_END , INITIALIZE_THIRD_FUNCTION_SET ,
		CONB_COMMAND_END , INITIALIZE_CLEAR_DISPLAY ,
		CONB_COMMAND_END , INITIALIZE_ENTRY_MODE_SET 
	};
	uint8_t* commandPointer = command;
			
	TimeControlInit();
	waitExecuteLcdCommand(LCD_COMMAND_LCD_RESET);														
	for (int i = 0; i < COMMAND_BUFFER_SIZE; i += 2 )
	{
		LcdI2cf0Write(LCD_SLAVE_ADDRESS, commandPointer + i, 2, lcdTransferEnd);
		lcdWaitI2cf0TransferEnd();
		if(i != COMMAND_BUFFER_POWER_STABLE) 
		{
			waitExecuteLcdCommand(LCD_COMMAND_ENTRY_MODE_SET);
		}
		if(i == COMMAND_BUFFER_POWER_STABLE)
		{
			waitExecuteLcdCommand(LCD_COMMAND_POWER_STABLE);
		}		  
	}
}
	
bool LcdDraw(uint8_t position, const char* str)
{
	//描画場所が範囲外なら描画しない。
	if(!LcdSetPositionForDisplay(position)) return false;
	//描画場所指定中にnackが返ってきたら描画しない。
	if(isNackOccurred()) return false;
	if(!LcdPutS(str)) return false;
	return true;
}

bool LcdSetPositionForDisplay(uint8_t position)
{
	uint8_t lcdPosition[2] = {CONB_COMMAND_END, 0};

	if(!createPosition(position,&lcdPosition[1])) return false;
	LcdI2cf0Write(LCD_SLAVE_ADDRESS, lcdPosition, sizeof(lcdPosition)/sizeof(uint8_t), lcdTransferEnd);
	waitUnitilEntireProcess(LCD_COMMAND_SET_DDRAM_ADDRESS);
	return true;
}

bool LcdPutS(const char* str)
{
	uint8_t strSize = 0;
	uint8_t string[MAX_OF_TXBUFFER_PUTS];
	
	if(!createStringData(str,string,&strSize)) return false;
	LcdI2cf0Write(LCD_SLAVE_ADDRESS, string, strSize, lcdTransferEnd);
	waitUnitilEntireProcess(LCD_COMMAND_WRITE_TO_DATA_RAM);
	return true;
}

void LcdClearDisplay(void)
{
	uint8_t clearDisplayCommand[2] = { CONB_COMMAND_END , 0x01};
	LcdI2cf0Write(LCD_SLAVE_ADDRESS, clearDisplayCommand, sizeof(clearDisplayCommand)/sizeof(uint8_t), lcdTransferEnd);
	waitUnitilEntireProcess(LCD_COMMAND_CLEAR_DISPLAY);
}

void LcdDisplayOnOff(uint8_t display, uint8_t cursor, uint8_t cursorBlink)
{
	uint8_t controlCursorDisplayCommand[2] = { CONB_COMMAND_END , (uint8_t)( 0x08 | (uint16_t)(display << 2) | (uint16_t)(cursor << 1) | (uint16_t)cursorBlink ) };
	LcdI2cf0Write(LCD_SLAVE_ADDRESS, controlCursorDisplayCommand, sizeof(controlCursorDisplayCommand)/sizeof(uint8_t), lcdTransferEnd);
	waitUnitilEntireProcess(LCD_COMMAND_DISPLAY_ON_OFF);
}

void LcdShiftCursorRight(void)
{
	uint8_t shiftCursorRightCommand[2] = { CONB_COMMAND_END , 0x14};
	LcdI2cf0Write(LCD_SLAVE_ADDRESS,shiftCursorRightCommand, sizeof(shiftCursorRightCommand)/sizeof(uint8_t), lcdTransferEnd);
	waitUnitilEntireProcess(LCD_COMMAND_CURSOR_OR_DISPLAY_SHIFT);
}


void LcdShiftCursorLeft(void)
{
	uint8_t shiftCursorLeftCommand[2] = { CONB_COMMAND_END , 0x10};
	LcdI2cf0Write(LCD_SLAVE_ADDRESS, shiftCursorLeftCommand, sizeof(shiftCursorLeftCommand)/sizeof(uint8_t), lcdTransferEnd);
	waitUnitilEntireProcess(LCD_COMMAND_CURSOR_OR_DISPLAY_SHIFT);
}

void LcdBacklightOn(void)
{
	OutputOnUInt32(&(PORT7->P7DO),BACKLIGHT_VALUE);
}

void LcdBacklightOff(void)
{
	OutputOffUInt32(&(PORT7->P7DO),BACKLIGHT_VALUE);
}





static bool createPosition(uint8_t givenPosition, uint8_t* calculatedPositon)
{
	//描画場所が範囲外なら描画しない
	if( ! ((LCD_START_OF_FIRST_LINE <= givenPosition) && (givenPosition <= LCD_END_OF_SECOND_LINE)))
	{
		return false;
	}
	//実際のDDRAMアドレスへ変換
	//二行目の場合の変換
	if((LCD_START_OF_SECOND_LINE <= givenPosition) && (givenPosition <= LCD_END_OF_SECOND_LINE))
	{
		givenPosition += LCD_SECOND_LINE_OFFSET;
	}
	givenPosition -= LCD_LOCATION_OFFSET;
	
	//コマンドセット
	givenPosition |= SET_DDRAM_ADDRESS;
	//データセット
	*calculatedPositon = givenPosition;
	return true;
}

static bool createStringData(const char* str, uint8_t* array, uint8_t* trasmitDataSize)
{
	//文字数が17以上なら失敗。
	*trasmitDataSize = (uint8_t)strlen(str);
	if( (*trasmitDataSize < DRAW_MIN) || (DRAW_MAX < *trasmitDataSize)) return false; 

	//NULL文字分
	*trasmitDataSize += 1;
	//データセット
	array[0] = CONB_DATA_END;
	memcpy(&array[1],str,*trasmitDataSize);
	return true;
}

static bool isNackOccurred(void)
{
	return i2cf0NackFlg;
}

static void waitUnitilEntireProcess(LCD_COMMAND cmd)
{
	lcdWaitI2cf0TransferEnd();
	waitExecuteLcdCommand(cmd);
}

static void lcdWaitI2cf0TransferEnd(void)
{
	i2cf0TransferEndFlg = false;
	while(!i2cf0TransferEndFlg)
	{
		wdt_clear();
	}
}

static void lcdTransferEnd( uint32_t size, uint8_t errStat )
{
	if(errStat == I2F_ERR_ACR) i2cf0NackFlg = true;
	else i2cf0NackFlg = false;
	i2cf0TransferEndFlg = true;
}
	
static void waitExecuteLcdCommand(LCD_COMMAND cmd)
{
	switch(cmd)
	{
		case LCD_COMMAND_LCD_RESET:
			TimeControlDelayMs(TIME_CONTROL_WAIT_RESET);
			break;
		
		case LCD_COMMAND_POWER_STABLE:
			TimeControlDelayMs(TIME_CONTROL_WAIT_POWER_STABLE);
			break;
		
		case LCD_COMMAND_CLEAR_DISPLAY:
		case LCD_COMMAND_RETURN_HOME:
			TimeControlDelayMs(TIME_CONTROL_5MS);
			break;
		
		case LCD_COMMAND_ENTRY_MODE_SET:
		case LCD_COMMAND_DISPLAY_ON_OFF:
		case LCD_COMMAND_FUNCTION_SET:
		case LCD_COMMAND_SET_DDRAM_ADDRESS:
		case LCD_COMMAND_SET_CGRAM:
		case LCD_COMMAND_WRITE_TO_DATA_RAM:
		case LCD_COMMAND_CURSOR_OR_DISPLAY_SHIFT:
		case LCD_COMMAND_INTERNAL_OSC_FREQUENCY:
		case LCD_COMMAND_SET_ICON_ADDRESS:
		case LCD_COMMAND_POWER_ICON_CONTROL_CONTRAST_SET:
		case LCD_COMMAND_FOLLOWER_CONTROL:
		case LCD_COMMAND_CONTRAST_SET:
			TimeControlDelayMs(TIME_CONTROL_1MS);
			break;	
	}
}
