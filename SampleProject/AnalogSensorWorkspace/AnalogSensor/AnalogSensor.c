/*****************************************************************************
 * File: AnalogSensor.c
 * Title: アナログセンサーモジュール
 * LastUpdated: 2025.05.27
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file AnalogSensor.c
 * @brief アナログセンサーモジュール
 * @details センサーのデータはバッファで管理する。
 */

#include "AnalogSensor.h"
#include "smpl_common.h"
#include "irq.h"
#include "saAdc0.h"
#include "wdt.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**< バッファのインデックスの初期値 */
#define RESET_BUFFER (0)
static const uint16_t TABLE_FREQ_TO_INTERVAL[] = 
{
	29980,	/**< 100Hz */
	14980,	/**< 200Hz */
	7480,	/**< 400Hz */
	3730,	/**< 800Hz */
	1855,	/**< 1600Hz */
	917,	/**< 3200Hz */
	449,	/**< 6400Hz */
	214,	/**< 12800Hz */
	97,		/**< 25600Hz */
};

/**< データ収集に使用しているバッファを指すポインタ */
volatile static int16_t* currentBuffer = NULL;
/**< データ収集に使用していたバッファを指すポインタ */
volatile static int16_t* previousBuffer = NULL;
volatile static bool toggleFlg = true;
/**< 1つ目のバッファ */
volatile static int16_t firstBuffer[ANALOG_SENSOR_BUFFER_SIZE];
/**< 2つ目のバッファ */
volatile static int16_t secondBuffer[ANALOG_SENSOR_BUFFER_SIZE];
/**< バッファのインデックス */
volatile static uint16_t bufferIndex = RESET_BUFFER;
/**< バッファの状態 */
volatile static ANALOG_SENSOR_BUFFER_STATE bufferState = ANALOG_SENSOR_BUFFER_STATE_IS_NOT_FULL;

/**
 * @brief センサー用バッファをクリアする。
 */
static void resetBuffer(void);

/**
 * @brief センサー用バッファの状態を管理する。
 */
inline static void manageBufferState(void);

void SAD_IRQHandler( void );


void AnalogSensorInit(ANALOG_SENSOR_SAMPLING_FREQUENCY frequency)
{
	initAdc_t  initAdc;
	enableAdcChannel_t enableChannel;
	
	if(ANALOG_SENSOR_SAMPLING_FREQUENCY_25600HZ < frequency) return;

	smpl_enablePeripheral(SAD0_PERI);

	__disable_irq();
	irq_sad0_dis();

	set_bit(PORT3->P3MOD0, (0 << 16));

	initAdc.discharge          = SAADC_SAINIT_DISCHARGE;
	initAdc.holdTime           = 0x03U;
	initAdc.clock              = SAADC_SACK_OSCLK_DIV16;
	initAdc.mode               = SAADC_SALP_CONTINUOUS;
	initAdc.limitInterrupt     = SAADC_SALMD_INSIDE_LIMIT;
	initAdc.limitMode          = SAADC_SALEN_DISABLE;
	initAdc.ampStabilityTime   = 0x02U;
	initAdc.interruptMode      = SAADC_SADIMD0_ALL_CH;
	initAdc.interruptLimitMode = SAADC_SADIMD1_LIMIT_MATCH;
	initAdc.interval           = TABLE_FREQ_TO_INTERVAL[frequency];
	initAdc.channelSync        = SAADC_SYNC_NORMAL;
	saAdc0_init( &initAdc );
	
	enableChannel.ch0  = SAADC_RUN;
	enableChannel.ch1  = SAADC_OFF;
	enableChannel.ch2  = SAADC_OFF;
	enableChannel.ch3  = SAADC_OFF;
	enableChannel.ch4  = SAADC_OFF;
	enableChannel.ch5  = SAADC_OFF;
	enableChannel.ch6  = SAADC_OFF;
	enableChannel.ch7  = SAADC_OFF;
	enableChannel.ch8  = SAADC_OFF;
	enableChannel.ch9  = SAADC_OFF;
	enableChannel.ch10 = SAADC_OFF;
	enableChannel.ch11 = SAADC_OFF;
	saAdc0_setEnableChannel( &enableChannel );
	
	resetBuffer();
	irq_sad0_clearIRQ();
	irq_sad0_ena();
	__enable_irq();
}

void AnalogSensorDefaultInit(void)
{
	AnalogSensorInit(ANALOG_SENSOR_SAMPLING_FREQUENCY_25600HZ);
}

void AnalogSensorStart(void)
{
	resetBuffer();
	saAdc0_start();
}

void AnalogSensorStop(void)
{
	saAdc0_stop();
}

ANALOG_SENSOR_BUFFER_STATE AnalogSensorStartUsingSensorBuffer(int16_t** sensorBuffer)
{
	__disable_irq();
	if(bufferState == ANALOG_SENSOR_BUFFER_STATE_IS_VALID)
	{
		*sensorBuffer = previousBuffer;
	}
	__enable_irq();
	return bufferState;
}

ANALOG_SENSOR_BUFFER_STATE AnalogSensorStopUsingSensorBuffer(void)
{
	__disable_irq();
	if(bufferState == ANALOG_SENSOR_BUFFER_STATE_IS_VALID)
	{
		bufferState = ANALOG_SENSOR_BUFFER_STATE_IS_NOT_FULL;
	}
	__enable_irq();
	return bufferState;
}

void SAD_IRQHandler( void )
{
	volatile uint16_t ui16Data = 0;	
	// 左詰め12bitのまま使ってフルスケール16bitとして扱う。
	ui16Data = (uint16_t)(saAdc0_getResult0() & 0xFFF0);	
	// ±反転と符号付きに変換。
	currentBuffer[bufferIndex] = (int16_t)(0x7FFF - ui16Data);

	if(bufferIndex >= (ANALOG_SENSOR_BUFFER_SIZE - 1))
	{
		bufferIndex = RESET_BUFFER;

		if(toggleFlg == true)
		{
			currentBuffer = secondBuffer;
			previousBuffer = firstBuffer;
			manageBufferState();
			toggleFlg = false;
		}
		else if(toggleFlg == false)
		{
			currentBuffer = firstBuffer;
			previousBuffer = secondBuffer;
			manageBufferState();
			toggleFlg = true;
		}
	}
	else
	{
		bufferIndex++;
	}
}


static void resetBuffer(void)
{
	memset(firstBuffer,0,sizeof(firstBuffer));
	memset(secondBuffer,0,sizeof(secondBuffer));
	currentBuffer = firstBuffer;
	previousBuffer = secondBuffer;
	toggleFlg = true;
	bufferIndex = RESET_BUFFER;
	bufferState = ANALOG_SENSOR_BUFFER_STATE_IS_NOT_FULL;
}


inline static void manageBufferState(void)
{
	if(bufferState == ANALOG_SENSOR_BUFFER_STATE_IS_NOT_FULL)
	{
		bufferState = ANALOG_SENSOR_BUFFER_STATE_IS_VALID;
		return;
	}

	if(bufferState == ANALOG_SENSOR_BUFFER_STATE_IS_VALID)
	{
		bufferState = ANALOG_SENSOR_BUFFER_STATE_IS_OVERFLOW;
		return;
	}
}
