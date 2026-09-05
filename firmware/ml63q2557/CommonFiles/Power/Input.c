/*****************************************************************************
 * File: Input.c
 * Title: 汎用入力を使う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Input.c
 * @brief 汎用入力を使う。
 */

#include "Input.h"
#include <stdio.h>

#define NUMBER_OF_INSTANCE				(INPUT_INDEX_END)

/**
 * @brief 読み取った入力状態が変化したかの列挙型
 */
typedef enum
{
	INPUT_STATE_UNCHANGED = 0,			/**< 変化なし */
	INPUT_STATE_CHANGED = 1,			/**< 変化あり */
	INPUT_STATE_WAIT_CHATTERING = 2,	/**< チャタリング待ち時間 */
	INPUT_STATE_END = 0xFF				/**< 1Byte */
}INPUT_STATE;

/**
 * @brief イベント管理用列挙型
 */
typedef enum
{
	EVNET_NOT_OCCURED = 0,				/**< イベント未発生 */
	EVENT_CREATED,						/**< イベント発生 */
}EVENT;

/**
 * @brief 入力のための構造体
 */
typedef struct
{
	volatile uint8_t InputStateFlg;
	volatile EVENT InputEventFlg;
	volatile uint8_t InputEventStatus;
	volatile uint8_t InputStatus;
	volatile uint8_t OldInputStatus;
	volatile uint8_t TmpInputStatus;
}INPUT_MANAGEMENT;

static INPUT_MANAGEMENT inputManeger[NUMBER_OF_INSTANCE];

/**
 * @brief indexの範囲外チェック。
 *
 * @param index 使用する機能のindex
 * @return bool 範囲外ならtrue
 */
static bool isIndexOutOfRange(INPUT_INDEX index);

/**
 * @brief チャタリング待ち時間の範囲外チェック。
 *
 * @param time 待ち時間
 * @return bool 範囲外ならtrue
 */
static bool isWaitingTimeOutOfRange(uint8_t time);

/**
 * @brief 値を読んで入力値が変化したことを確認する。
 *
 * @param index 使用する機能のindex
 * @param input 読んだ値
 * @param time 待ち時間
 */
inline static void readInput(INPUT_INDEX index, uint8_t input, uint8_t time);



bool InputInit(INPUT_INDEX index, uint8_t initStatus)
{
	if(isIndexOutOfRange(index))return false;
	inputManeger[index].InputStateFlg = INPUT_STATE_UNCHANGED;
	inputManeger[index].InputEventFlg = EVNET_NOT_OCCURED;
	inputManeger[index].InputEventStatus = initStatus;
	inputManeger[index].InputStatus = initStatus;
	inputManeger[index].OldInputStatus = initStatus;
	inputManeger[index].TmpInputStatus = initStatus;
	return true;
}

INPUT_POLLING_RESULT InputPolling(INPUT_INDEX index, uint8_t waitingTime, InputGetTargetValue getValue, InputFunc func)
{
	if( getValue == NULL ) return INPUT_POLLING_RESULT_INVALID_ARGUMENT;
	if( func == NULL ) return INPUT_POLLING_RESULT_INVALID_ARGUMENT;
	if( isIndexOutOfRange(index) ) return INPUT_POLLING_RESULT_INVALID_ARGUMENT;
	if( isWaitingTimeOutOfRange(waitingTime) ) return INPUT_POLLING_RESULT_INVALID_ARGUMENT;
	readInput( index, getValue(),waitingTime);
	if( inputManeger[index].InputEventFlg >= EVENT_CREATED )
	{
		func();
		inputManeger[index].InputEventFlg = EVNET_NOT_OCCURED;
		return INPUT_POLLING_RESULT_EVENT_OCCURED;
	}
	return INPUT_POLLING_RESULT_NORMAL;
}

uint8_t InputGetSystemInputValue(INPUT_INDEX index)
{
	return inputManeger[index].InputEventStatus;
}



static bool isIndexOutOfRange(INPUT_INDEX index)
{
	if( index >= (sizeof(inputManeger) / sizeof(INPUT_MANAGEMENT)) ) return true;
	else return false;
}

static bool isWaitingTimeOutOfRange(uint8_t time)
{
	if( time < INPUT_STATE_WAIT_CHATTERING ) return true;
	else return false;
}

inline static void readInput(INPUT_INDEX index,uint8_t input, uint8_t time)
{
	inputManeger[index].OldInputStatus = inputManeger[index].InputStatus;
	inputManeger[index].InputStatus = input;
	
	if(!inputManeger[index].InputStateFlg)
	{
		if( inputManeger[index].InputStatus ^ inputManeger[index].OldInputStatus )
		{
			inputManeger[index].InputStateFlg = INPUT_STATE_CHANGED;	
			inputManeger[index].TmpInputStatus = inputManeger[index].InputStatus;
			return;
		}
	}
	
	//チャタリング調整
	if( 	( inputManeger[index].InputStateFlg >= INPUT_STATE_CHANGED )
			&&( inputManeger[index].InputStateFlg < time ))
	{
		inputManeger[index].InputStateFlg += 1;
		return;
	}
	
	if( inputManeger[index].InputStateFlg >= time)
	{
		if( ! ( inputManeger[index].InputStatus ^ inputManeger[index].TmpInputStatus ) )
		{
			inputManeger[index].InputStateFlg = INPUT_STATE_UNCHANGED;
			inputManeger[index].InputEventFlg = EVENT_CREATED;
			inputManeger[index].InputEventStatus = inputManeger[index].InputStatus;	
		}
		else
		{
			inputManeger[index].InputStateFlg = INPUT_STATE_CHANGED;
			inputManeger[index].TmpInputStatus = inputManeger[index].InputStatus;
			inputManeger[index].InputEventFlg = EVNET_NOT_OCCURED;
		}
	}
}

