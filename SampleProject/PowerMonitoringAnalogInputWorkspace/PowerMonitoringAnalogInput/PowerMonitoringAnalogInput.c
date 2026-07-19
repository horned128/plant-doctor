/*****************************************************************************
 * File: PowerMonitoringAnalogInput.c
 * Title: 電源電圧監視アナログ入力を使用する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PowerMonitoringAnalogInput.c
 * @brief 電源電圧監視アナログ入力を使用する。
 */

#include "PowerMonitoringAnalogInput.h"
#include "Output.h"
#include "smpl_common.h"
#include "saAdc_common.h"
#include "saAdc1.h"
#include "irq.h"
#include "wdt.h"
#include "TimeControl.h"
#include "float.h"
#include "math.h"

/**< IOの設定 */
#define INPUT_CONTROL_PORT_CONFIG			(0x02 << 8)
#define INPUT_CONTROL_PORT_VALUE			(0x02)

/**< ADの設定 */
#define ANALOG_INPUT_CONFIG					(0x00 << 24)
#define ONESHOT_INTERVAL					(0)

/**< TPS22919がONになるまでの時間 */
#define WAIT_2MS							(2)

/**< 分解能 */
#define NUMBER_OF_DIVISIONS					(4096)

/**< 基準 */
#define POWER_MAX							(6.6f)

volatile static bool isADInterruptOccurred = false;
volatile static float currentVoltageValue = 0.0f;

/**
 * @brief アナログ入力機能を初期化する。
 */
static void analogInit(void);

/**
 * @brief 電源電圧値を計算する。
 *
 * @return float 電源電圧値。
 */
inline static float calculateVoltageValue(void);

/**
 * @brief ADCから値を取得する。
 *
 * @return uint32_t AD変換値。
 */
inline static uint32_t getAnalogVoltageValue(void);

void SAD1_IRQHandler(void);

/**
 * @brief TPS22919をONにする。
 */
static void inputControlPortOn(void);

/**
 * @brief TPS22919をOFFにする。
 */
static void inputControlPortOff(void);



void PowerMonitoringAnalogInputInit(void)
{
	//TPS22919をOFFにする。
	set_bit(PORT3->P3MOD0, INPUT_CONTROL_PORT_CONFIG);
	inputControlPortOff();
	
	TimeControlInit();
	
	//ad初期化
	analogInit();
}

float PowerMonitoringAnalogInputGetVoltageValue(void)
{
	float voltageValue = 0;
	
	//電圧監視ゲートをOn
	inputControlPortOn();
	
	//2ms待つ
	TimeControlDelayMs(WAIT_2MS);
	
	//電源電圧値を取得
	voltageValue = calculateVoltageValue();
	
	//電圧監視ゲートをoff
	inputControlPortOff();
	
	//現在の値を更新
	currentVoltageValue = voltageValue;
	return voltageValue;
}

float PowerMonitoringAnalogInputCheckCurrentVoltageValue(void)
{
	return currentVoltageValue;
}


void SAD1_IRQHandler(void)
{
	isADInterruptOccurred = true;
}




static void analogInit(void)
{
	initAdc_t initStatus;
	enableAdcChannel_t channelStatus;
	
	smpl_enablePeripheral(SAD1_PERI);
	
	__disable_irq();
	irq_sad1_dis();

	set_bit(PORT3->P3MOD0, ANALOG_INPUT_CONFIG);
	
	initStatus.discharge			= SAADC_SAINIT_DISCHARGE;
	initStatus.holdTime				= 0x03;
	initStatus.clock				= SAADC_SACK_OSCLK_DIV16;
	initStatus.mode					= SAADC_SALP_ONESHOT; 
	initStatus.limitInterrupt		= SAADC_SALMD_INSIDE_LIMIT;
	initStatus.limitMode			= SAADC_SALEN_DISABLE;
	initStatus.ampStabilityTime		= 0x02;
	initStatus.interruptMode		= SAADC_SADIMD0_ALL_CH;
	initStatus.interruptLimitMode	= SAADC_SADIMD1_LIMIT_MATCH;
	initStatus.interval				= ONESHOT_INTERVAL;
	initStatus.channelSync			= SAADC_SYNC_NORMAL;
	saAdc1_init(&initStatus);
	
	channelStatus.ch0	= SAADC_OFF;
	channelStatus.ch1	= SAADC_RUN;
	channelStatus.ch2	= SAADC_OFF;
	channelStatus.ch3	= SAADC_OFF;
	channelStatus.ch4	= SAADC_OFF;
	channelStatus.ch5	= SAADC_OFF;
	channelStatus.ch6	= SAADC_OFF;
	channelStatus.ch7	= SAADC_OFF;
	channelStatus.ch8	= SAADC_OFF;
	channelStatus.ch9	= SAADC_OFF;
	channelStatus.ch10	= SAADC_OFF;
	channelStatus.ch11	= SAADC_OFF;
	saAdc1_setEnableChannel(&channelStatus);
	
	isADInterruptOccurred = false;
	currentVoltageValue = 0.0f;

	irq_sad1_setLevel(0);
	irq_sad1_clearIRQ();
	irq_sad1_ena();
	__enable_irq();
}

inline static float calculateVoltageValue(void)
{
	uint32_t adcValue = 0;
	float voltageValue = 0;
	
	//AD変換
	adcValue = getAnalogVoltageValue();

	//電圧変換
	voltageValue = (float)adcValue * (POWER_MAX / NUMBER_OF_DIVISIONS);

	return voltageValue;
}

inline static uint32_t getAnalogVoltageValue(void)
{
	uint32_t value;
	
	//AD変換開始
	saAdc1_start();
	while(!isADInterruptOccurred) wdt_clear();
	
	//AD終了
	saAdc1_stop();
	isADInterruptOccurred = false;
	
	//12bitAD右詰 左20bit不使用
	value = (saAdc1_getResult1() >> 4);
	return value;
}


static void inputControlPortOn(void)
{
	OutputOnUInt32(&(PORT3->P3DO),INPUT_CONTROL_PORT_VALUE);
}

static void inputControlPortOff(void)
{
	OutputOffUInt32(&(PORT3->P3DO),INPUT_CONTROL_PORT_VALUE);
}
