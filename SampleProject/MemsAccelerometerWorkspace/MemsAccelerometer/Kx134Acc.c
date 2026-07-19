/*****************************************************************************
 * File: Kx134Acc.c
 * Title: MEMS加速度センサーKx134-1211を制御するモジュール
 * LastUpdated: 2025.05.27
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Kx134Acc.c
 * @brief MEMS加速度センサーKx134-1211を制御するモジュール
 * @details センサーのデータはバッファで管理する。
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "ssiof_common.h"
#include "Kx134Acc.h"
#include "Kx134Spi.h"
#include "ssiof0.h"
#include "wdt.h"
#include "irq.h"
#include "TimeControl.h"
#include <stdio.h>
#include <string.h>

/**< ダミーデータ */
#define DUMMY_DATA						(0xff)
/**< Readコマンド */
#define READ_COMMAND					(0x80)
/**< レジスタ */
#define REG_XOUT_L						(0x08)
#define REG_XOUT_H						(0x09)
#define REG_YOUT_L						(0x0A)
#define	REG_YOUT_H						(0x0B)
#define REG_ZOUT_L						(0x0C)
#define REG_ZOUT_H						(0x0D)
/**< モード設定 */
#define REG_CNTL1						(0x1B)
#define STANDBY_MODE					(0x00)
#define OPERATING_MODE					(0x80)
#define LOW_POEWER_MODE					(0x00)
#define HIGH_PEFORMANCE_MODE			(0x40)
#define DATA_REDAY_ENGINE_ENABLE		(0x20)
#define GSEL							(0x00)
#define BEFORE_STARTUP_CNTL1			(STANDBY_MODE | HIGH_PEFORMANCE_MODE  | GSEL)
#define DEFAULT_CNTL1					(BEFORE_STARTUP_CNTL1 | OPERATING_MODE | DATA_REDAY_ENGINE_ENABLE) 
/**< LPFとサンプリング周波数の設定 */
#define REG_ODCNTL						(0x21)
#define DEFAULT_ODCNTL					(0)
/**< 割り込み設定 */
#define REG_INC1						(0x22)
#define PW1_NO_USE_PULSE				(0xC0)
#define IEN1_INTERRUPT_ENABLED			(0x20)
#define IEA1_ACTIVE_LOW					(0x00)
#define IEL1_READ_CLEAR					(0x00)
#define IEL1_PULSE_CREATE				(0x08)
#define SPI3E_DISABLED					(0x00)
#define DEFAULT_INC1					(PW1_NO_USE_PULSE | IEN1_INTERRUPT_ENABLED | IEA1_ACTIVE_LOW | IEL1_READ_CLEAR | SPI3E_DISABLED)
/**< 割り込みの種類の設定 */
#define REG_INC4						(0x25)
#define DATA_READY_INTERRUPT			(0x10)
#define DEFAULT_INC4					(DATA_READY_INTERRUPT)
/**< スタートアップタイム */
#define START_UP_TIME_300MS				(300)
#define START_UP_TIME_1000MS			(1000)
/**< パワーアップタイム */
#define POWER_UP_TIME					(50)


/**< バッファのインデックスの初期値 */
#define RESET_BUFFER 					(0)
/**< センサー初期化コマンドサイズ */
#define INITIALIZE_COMMAND_SIZE			(1)
/**< センサーデータ取得コマンドサイズ */
#define GET_SENSOR_DATA_COMMAND_SIZE	(2)
/**< Spi通信受信用バッファサイズ */
#define BUFFER_SIZE						(4)

/**
 * @brief MEMS加速度センサーに対する設定を管理する構造体
 */
typedef struct
{
	uint16_t CommandForSensor[GET_SENSOR_DATA_COMMAND_SIZE];	/**< 1軸データ取得時コマンド */
	KX134_ACC_SAMPLING_FREQUENCY Frequency;						/**< MEMS加速度センサーのサンプリング周波数 */
	KX134_ACC_LPF Filter;										/**< MEMS加速度センサーのLPF */
}KX134_ACC_SETTINGS;
static KX134_ACC_SETTINGS localSettings;


/**< データ収集に使用しているバッファを指すポインタ */
volatile static int16_t* currentBuffer = NULL;
/**< データ収集に使用していたバッファを指すポインタ */
volatile static int16_t* previousBuffer = NULL;
volatile static bool toggleFlg = true;
/**< 1つ目のバッファ */
volatile static int16_t firstBuffer[KX134_ACC_BUFFER_SIZE];
/**< 2つ目のバッファ */
volatile static int16_t secondBuffer[KX134_ACC_BUFFER_SIZE];
/**< バッファのインデックス */
volatile static uint16_t bufferIndex = RESET_BUFFER;
/**< バッファの状態 */
volatile static KX134_ACC_BUFFER_STATE bufferState = KX134_ACC_BUFFER_STATE_IS_NOT_FULL;

/**< Spi通信受信用バッファ */
static uint16_t rxBuffer[BUFFER_SIZE]; 
/**< Spi同期待ち用フラグ */
volatile static bool spiTransferEndFlag = false;


/**
 * @brief MEMS加速度センサーの設定を与える。
 *
 * @param type MEMS加速度センサーの軸
 * @param frequency MEMS加速度センサーのサンプリング周波数
 * @param filter MEMS加速度センサーのLPF
 */
static void configInit(KX134_ACC_AXIS type, KX134_ACC_SAMPLING_FREQUENCY frequency, KX134_ACC_LPF filter);

/**
 * @brief センサーデータ取得コマンドをリセットする。
 */
static void resetAxis(void);

/**
 * @brief 軸に応じて適切なセンサーデータ取得コマンドを作成する。
 *
 * @param type センサーの軸
 */
static void setAxis(KX134_ACC_AXIS type);

/**
 * @brief Spi送受信を行う
 *
 * @param txData 送信データ
 * @param size 送信データのサイズ
 * @param func Spi通信後の転送終了割り込みで行うコールバック。無い場合はNULL
 */
inline static void sendReceive(uint16_t* txData,uint32_t size, cbfSsiof_t func);

/**
 * @brief Spi送受信が完了するまで同期待ちを行う。
 */
static void waitSpiTransferEnd(void);

/**
 * @brief Spi送受信が完了した時のコールバック
 */
static void spiTansferEndInterrupt(uint32_t dataCnt, uint16_t errStatus);

/**
 * @brief kx134-1211センサーに対して設定を行う。
 */
static void setSensorConfig(void);

/**
 * @brief Spiのセンサーデータ読み取り可能割り込みにて、センサーデータを取得する。
 */
static void getSensorValue(void);

/**
 * @brief Spiで受信したセンサーデータをバッファで管理する。
 */
inline static void manageSensorData(void);

/**
 * @brief センサー用バッファをクリアする。
 */
static void resetBuffer(void);

/**
 * @brief センサー用バッファの状態を管理する。
 */
inline static void manageBufferState(void);




void Kx134AccInit(KX134_ACC_AXIS type, KX134_ACC_SAMPLING_FREQUENCY frequency, KX134_ACC_LPF filter)
{
	//加速度センサー用CPUレジスタ初期化
	Kx134SpiPeripheralInit();
	
	//設定初期化
	configInit(type,frequency,filter);

	resetBuffer();
	//センサーの起動待ち Max1300ms
	TimeControlInit();
	TimeControlDelayMs(START_UP_TIME_1000MS);
	TimeControlDelayMs(START_UP_TIME_300MS);
}

void Kx134AccDefaultInit(void)
{
	Kx134AccInit(KX134_ACC_AXIS_Z,KX134_ACC_SAMPLING_FREQUENCY_6400HZ,KX134_ACC_LPF_ODR_2);
}

void Kx134AccStart(void)
{
	//センサーデータ読み取り割り込み禁止、Spi割り込み許可
	Kx134SpiSensorDataReadyInterruptUnuse();
	Kx134SpiInterruptUnuse();
	Kx134SpiInterruptUse();
	
	//バッファリセット
	resetBuffer();

	//センサー初期化
	setSensorConfig();

	//センサーデータ読み取り可能割り込み開始
	Kx134SpiSensorDataReadyInterruptUse();
}

void Kx134AccStop(void)
{
	Kx134SpiInterruptUnuse();
	Kx134SpiSensorDataReadyInterruptUnuse();
}

KX134_ACC_BUFFER_STATE Kx134AccStartUsingSensorBuffer(int16_t** sensorBuffer)
{
	__disable_irq();
	if(bufferState == KX134_ACC_BUFFER_STATE_IS_VALID)
	{
		*sensorBuffer = previousBuffer;
	}
	__enable_irq();
	return bufferState;
}

KX134_ACC_BUFFER_STATE Kx134AccStopUsingSensorBuffer(void)
{
	__disable_irq();
	if(bufferState == KX134_ACC_BUFFER_STATE_IS_VALID)
	{
		bufferState = KX134_ACC_BUFFER_STATE_IS_NOT_FULL;
	}
	__enable_irq();
	return bufferState;
}




static void configInit(KX134_ACC_AXIS type, KX134_ACC_SAMPLING_FREQUENCY frequency, KX134_ACC_LPF filter)
{
	if(KX134_ACC_AXIS_Z < type) return;
	if( (frequency < KX134_ACC_SAMPLING_FREQUENCY_100HZ) || (KX134_ACC_SAMPLING_FREQUENCY_25600HZ < frequency) ) return;
	if(KX134_ACC_LPF_ODR_2 < filter) return;	
	
	//軸
	resetAxis();
	setAxis(type);
	//サンプリング周波数
	localSettings.Frequency = frequency;
	//lpf
	localSettings.Filter = filter;
	//センサーデータ読み取り可能割り込み
	Kx134SpiSetSensorDataReadyInterrupt(getSensorValue);
}

static void resetAxis(void)
{
	localSettings.CommandForSensor[0] = (( ( 0 | READ_COMMAND ) << 8 ) | DUMMY_DATA);
	localSettings.CommandForSensor[1] = ( ( DUMMY_DATA << 8 ) | DUMMY_DATA );
}

static void setAxis(KX134_ACC_AXIS type)
{
	switch(type)
	{
		case KX134_ACC_AXIS_X:
			localSettings.CommandForSensor[0] |= ( REG_XOUT_L << 8 );
			break;
		case KX134_ACC_AXIS_Y:
			localSettings.CommandForSensor[0] |= ( REG_YOUT_L << 8 );
			break;
		case KX134_ACC_AXIS_Z:
			localSettings.CommandForSensor[0] |= ( REG_ZOUT_L << 8 );
			break;
	}
}

inline static void sendReceive(uint16_t* txData,uint32_t size,cbfSsiof_t func)
{
	spiTransferEndFlag = false;
	Kx134SpiClearFifo();
	Kx134SpiStart(rxBuffer, txData, size,func);
}
 
static void waitSpiTransferEnd(void)
{
	while(spiTransferEndFlag == false) 
	{
		wdt_clear();
	}
}

static void spiTansferEndInterrupt(uint32_t dataCnt, uint16_t errStatus)
{
	spiTransferEndFlag = true;
}

static void setSensorConfig(void)
{
	//first
	uint16_t txDataCntl1First = ( ( REG_CNTL1 << 8 ) | 0); 
	uint16_t txDataInc1 = ( ( REG_INC1 << 8 ) | DEFAULT_INC1 );
	uint16_t txDataInc4 = ( ( REG_INC4 << 8) | DEFAULT_INC4 );
	uint16_t txDataOdcntl = ( ( REG_ODCNTL << 8 ) | DEFAULT_ODCNTL );
	//last
	uint16_t txDataCntl1Second = ( ( REG_CNTL1 << 8) | DEFAULT_CNTL1 );
	
	//LPFとサンプリング周波数
	txDataOdcntl |=  ( ( localSettings.Filter << 6 ) | localSettings.Frequency);
	
	//CNTL1 first
	sendReceive(&txDataCntl1First,INITIALIZE_COMMAND_SIZE,spiTansferEndInterrupt);
	waitSpiTransferEnd();

	//INC1
	sendReceive(&txDataInc1,INITIALIZE_COMMAND_SIZE,spiTansferEndInterrupt);
	waitSpiTransferEnd();
	
	//INC4
	sendReceive(&txDataInc4,INITIALIZE_COMMAND_SIZE,spiTansferEndInterrupt);
	waitSpiTransferEnd();
	
	//ODCTL
	sendReceive(&txDataOdcntl,INITIALIZE_COMMAND_SIZE,spiTansferEndInterrupt);
	waitSpiTransferEnd();

	//CNTL1 last
	sendReceive(&txDataCntl1Second,INITIALIZE_COMMAND_SIZE,spiTansferEndInterrupt);
	waitSpiTransferEnd();
	
	//設定反映待ち Max50ms
	TimeControlDelayMs(POWER_UP_TIME);
}

static void getSensorValue(void)
{
	sendReceive(localSettings.CommandForSensor,BUFFER_SIZE,spiTansferEndInterrupt);
	waitSpiTransferEnd();
	manageSensorData();
}

inline static void manageSensorData(void)
{
	volatile uint16_t ui16Data = 0;
	ui16Data = ( (rxBuffer[1] & 0xff00 ) | ( rxBuffer[0] & 0x00ff ) );
	currentBuffer[bufferIndex] = (int16_t)ui16Data;

	if(bufferIndex >= (KX134_ACC_BUFFER_SIZE - 1))
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
	bufferState = KX134_ACC_BUFFER_STATE_IS_NOT_FULL;
}


inline static void manageBufferState(void)
{
	if(bufferState == KX134_ACC_BUFFER_STATE_IS_NOT_FULL)
	{
		bufferState = KX134_ACC_BUFFER_STATE_IS_VALID;
		return;
	}

	if(bufferState == KX134_ACC_BUFFER_STATE_IS_VALID)
	{
		bufferState = KX134_ACC_BUFFER_STATE_IS_OVERFLOW;
		return;
	}
}
