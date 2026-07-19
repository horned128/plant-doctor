/*****************************************************************************
 * File: PhotoCouplerInput.c
 * Title: フォトカプラ入力を取り扱う。
 * LastUpdated: 2025.06.02
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PhotoCouplerInput.c
 * @brief フォトカプラ入力を取り扱う。\n
 *		  回路図のISOIN0をフォトカプラ入力0とする。\n
 *		  回路図のISOIN1をフォトカプラ入力1とする。
 */

#include "PhotoCouplerInput.h"
#include "smpl_common.h"
#include "Output.h"

#define PHOTO_COUPLER_INPUT_CONFIG	( (0x01U << 8U) | (0x01U << 0) )
#define PHOTO_COUPLER_INPUT0_PUSH	(1 << 4)
#define PHOTO_COUPLER_INPUT1_PUSH	(1 << 5)
#define PHOTO_COUPLER_INPUT			(0x30)
#define WAITING_TIME				(2)
#define PULLUP						(0x30)

volatile static bool photoCouplerInput0StatusFlg = false;
volatile static bool photoCouplerInput1StatusFlg = false;

/**
 * @brief 入力値を読み取る。
 *
 * @return uint8_t 
 */
inline static uint8_t readPhotoCouplerInput(void);

/**
 * @brief 入力値の変化に応じてフラグ管理を行う。
 */
static void managePhotoCouplerInput(void);



void PhotoCouplerInputInit(void)
{
	set_reg32(PORT6->P6MOD1,PHOTO_COUPLER_INPUT_CONFIG);
	InputInit(INPUT_INDEX_PHOTO_COUPLER_INPUT,PULLUP);
	photoCouplerInput0StatusFlg = false;
	photoCouplerInput1StatusFlg = false;
}

INPUT_POLLING_RESULT PhotoCouplerInputPolling(void)
{
	return InputPolling(INPUT_INDEX_PHOTO_COUPLER_INPUT,WAITING_TIME,readPhotoCouplerInput,managePhotoCouplerInput);
}

bool PhotoCouplerInput0IsEntered(void)
{
	return photoCouplerInput0StatusFlg;
}

bool PhotoCouplerInput1IsEntered(void)
{
	return photoCouplerInput1StatusFlg;
}



inline static uint8_t readPhotoCouplerInput(void)
{
	return (uint8_t) ( ( PORT6->P6DI ) & PHOTO_COUPLER_INPUT );
}

static void managePhotoCouplerInput(void)
{
	//PullUpなので逆
	//フォトカプラ入力0
	if( ! ( InputGetSystemInputValue(INPUT_INDEX_PHOTO_COUPLER_INPUT) & PHOTO_COUPLER_INPUT0_PUSH) )
	{	
		photoCouplerInput0StatusFlg = true;
	}
	else
	{
		photoCouplerInput0StatusFlg = false;
	}	
	//フォトカプラ入力1
	if( ! ( InputGetSystemInputValue(INPUT_INDEX_PHOTO_COUPLER_INPUT) & PHOTO_COUPLER_INPUT1_PUSH) )
	{	
		photoCouplerInput1StatusFlg = true;
	}
	else
	{
		photoCouplerInput1StatusFlg = false;
	}	
	
}

