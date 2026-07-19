/*****************************************************************************
 * File: Sw.c
 * Title: Sw(プッシュスイッチ/ディップスイッチ)を使う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Sw.c
 * @brief Sw(プッシュスイッチ/ディップスイッチ)を使う。\n
          プッシュスイッチは回路図のSW2を1、SW3を2、SW4を3、SW5を4とする。\n
          ディップスイッチは回路図のSW1の1を1、2を2、3を3、4を4とする。
 */

#include <stdint.h>
#include "Sw.h"
#include "smpl_common.h"

typedef enum
{
	PUSH_DSW4 = ( 1 << 7 ),
	PUSH_DSW3 = ( 1 << 6 ),
	PUSH_DSW2 = ( 1 << 5 ),
	PUSH_DSW1 = ( 1 << 4 ),
	PUSH_PSW4 = ( 1 << 3 ),
	PUSH_PSW3 = ( 1 << 2 ),
	PUSH_PSW2 = ( 1 << 1 ),
	PUSH_PSW1 = ( 1 << 0 )
}PUSH;

#define WAITING_TIME	(2)
#define PULLUP			(0xff)

#define DSW_CONFIG		( ( 0x01U << 24U ) | ( 0x01U << 16U ) | ( 0x01U << 8U) | ( 0x01U << 0 ) )
#define PSW_CONFIG		( ( 0x01U << 24U ) | ( 0x01U << 16U ) | ( 0x01U << 8U) | ( 0x01U << 0 ) )

/**
 * @brief 長押し
 */
volatile static bool keepPsw1PressedFlg = false;
volatile static bool keepPsw2PressedFlg = false;
volatile static bool keepPsw3PressedFlg = false;
volatile static bool keepPsw4PressedFlg = false;
volatile static bool keepDsw1PressedFlg = false;
volatile static bool keepDsw2PressedFlg = false;
volatile static bool keepDsw3PressedFlg = false;
volatile static bool keepDsw4PressedFlg = false;

/**
 * @brief 短押し
 */
volatile static bool iskeepPsw1Enable = true;
volatile static bool iskeepPsw2Enable = true;
volatile static bool iskeepPsw3Enable = true;
volatile static bool iskeepPsw4Enable = true;


/**
 * @brief スイッチの入力値を読み取る。
 */
inline static uint8_t readDipPushSw(void);

/**
 * @brief プッシュスイッチの入力値を読み取る。
 */
inline static uint8_t readPsw(void);

/**
 * @brief ディップスイッチの入力値を読み取る。
 */
inline static uint8_t readDsw(void);

/**
 * @brief スイッチの入力値の変化に応じてフラグ管理を行う。
 */
inline static void manageFlg(void);

/**
 * @brief プッシュスイッチの入力値の変化に応じてフラグ管理を行う。
 */
inline static void managePswFlg(void);

/**
 * @brief ディップスイッチの入力値の変化に応じてフラグ管理を行う。
 */
inline static void manageDswFlg(void);
		
		
		
		
				
void SwInit(void)
{
	set_reg32(PORT3->P3MOD1, DSW_CONFIG);
	set_reg32(PORT5->P5MOD0, PSW_CONFIG);
	
	InputInit(INPUT_INDEX_DIP_PSH,PULLUP);
	keepPsw1PressedFlg = false;
	keepPsw2PressedFlg = false;
	keepPsw3PressedFlg = false;
	keepPsw4PressedFlg = false;
	keepDsw1PressedFlg = false;
	keepDsw2PressedFlg = false;
	keepDsw3PressedFlg = false;
	keepDsw4PressedFlg = false;
	
	iskeepPsw1Enable = true;
	iskeepPsw2Enable = true;
	iskeepPsw3Enable = true;
	iskeepPsw4Enable = true;
}

INPUT_POLLING_RESULT SwPolling(void)
{
	return InputPolling(INPUT_INDEX_DIP_PSH,WAITING_TIME,readDipPushSw,manageFlg);
}

bool SwIsPsw1Entered(void)
{
	if(iskeepPsw1Enable) return keepPsw1PressedFlg;
	else return false;
}

bool SwIsPsw2Entered(void)
{
	if(iskeepPsw2Enable) return keepPsw2PressedFlg;
	else return false;
}

bool SwIsPsw3Entered(void)
{
	if(iskeepPsw3Enable) return keepPsw3PressedFlg;
	else return false;
}

bool SwIsPsw4Entered(void)
{
	if(iskeepPsw4Enable) return keepPsw4PressedFlg;
	else return false;
}

void SwDisablePsw1UntilNextPress(void)
{
	__disable_irq();
	if(SwIsPsw1Entered())
	{
		keepPsw1PressedFlg = false;
		iskeepPsw1Enable = false;
	}
	__enable_irq();
}

void SwDisablePsw2UntilNextPress(void)
{
	__disable_irq();
	if(SwIsPsw2Entered())
	{
		keepPsw2PressedFlg = false;
		iskeepPsw2Enable = false;
	}
	__enable_irq();
}

void SwDisablePsw3UntilNextPress(void)
{
	__disable_irq();
	if(SwIsPsw3Entered())
	{
		keepPsw3PressedFlg = false;
		iskeepPsw3Enable = false;
	}
	__enable_irq();
}

void SwDisablePsw4UntilNextPress(void)
{
	__disable_irq();
	if(SwIsPsw4Entered())
	{
		keepPsw4PressedFlg = false;
		iskeepPsw4Enable = false;
	}
	__enable_irq();
}

void SwDisableAllPswUntilNextPress(void)
{
	SwDisablePsw1UntilNextPress();
	SwDisablePsw2UntilNextPress();
	SwDisablePsw3UntilNextPress();
	SwDisablePsw4UntilNextPress();
}

bool SwIsDsw1Entered(void)
{
	return keepDsw1PressedFlg;
}

bool SwIsDsw2Entered(void)
{
	return keepDsw2PressedFlg;
}

bool SwIsDsw3Entered(void)
{
	return keepDsw3PressedFlg;
}

bool SwIsDsw4Entered(void)
{
	return keepDsw4PressedFlg;
}





inline static uint8_t readDipPushSw(void)
{
	return ( readDsw() + readPsw() );
}

inline static uint8_t readPsw(void)
{
	return (uint8_t) ( ( PORT5->P5DI ) & 0x0f );
}

inline static uint8_t readDsw(void)
{
	return (uint8_t) ( ( PORT3->P3DI ) & 0xf0 );
}

inline static void manageFlg(void)
{
	managePswFlg();
	manageDswFlg();
}

inline static void managePswFlg(void)
{
	//PSW1
	if( ! ( InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_PSW1) )
	{	
		keepPsw1PressedFlg = true;
	}
	else
	{
		keepPsw1PressedFlg = false;
		iskeepPsw1Enable = true;
	}	
	
	//PSW2
	if( ! ( InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_PSW2) )
	{	
		keepPsw2PressedFlg = true;
	}
	else
	{
		keepPsw2PressedFlg = false;
		iskeepPsw2Enable = true;
	}	
		
	//PSW3
	if( ! ( InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_PSW3) )
	{
		keepPsw3PressedFlg = true;
	}
	else
	{
		keepPsw3PressedFlg = false;
		iskeepPsw3Enable = true;
	}	
	
	//PSW4
	if( ! (InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_PSW4) )
	{			
		keepPsw4PressedFlg = true;
	}
	else
	{
		keepPsw4PressedFlg = false;
		iskeepPsw4Enable = true;
	}
}

inline static void manageDswFlg(void)
{
	//DSW1
	if( ! (InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_DSW1) ) 
	{
		keepDsw1PressedFlg = true;
	}
	else
	{
		keepDsw1PressedFlg = false;
	}	
	
	//DSW2
	if( ! (InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_DSW2) )
	{
		keepDsw2PressedFlg = true;
	}
	else
	{
		keepDsw2PressedFlg = false;
	}
	
	//DSW3
	if( ! (InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_DSW3) )
	{
		keepDsw3PressedFlg = true;
	}
	else
	{
		keepDsw3PressedFlg = false;
	}
	
	//DSW4
	if( ! (InputGetSystemInputValue(INPUT_INDEX_DIP_PSH) & PUSH_DSW4) )
	{
		keepDsw4PressedFlg = true;
	}
	else
	{
		keepDsw4PressedFlg = false;
	}
}

