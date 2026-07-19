/*****************************************************************************
 * File: AnalogSensor.h
 * Title: アナログセンサーモジュール
 * LastUpdated: 2025.05.30
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file AnalogSensor.h
 * @brief アナログセンサーモジュール
 * @details センサーのデータはバッファで管理する。
 */

#ifndef ANALOG_SENSOR_H__
#define ANALOG_SENSOR_H__
#include <stdint.h>

/**
 * @brief ADCのサンプリング周波数の列挙型
 */
typedef enum
{
	ANALOG_SENSOR_SAMPLING_FREQUENCY_100HZ = 0,		/**< 100Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_200HZ,			/**< 200Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_400HZ,			/**< 400Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_800HZ,			/**< 800Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_1600HZ,		/**< 1600Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_3200HZ,		/**< 3200Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_6400HZ,		/**< 6400Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_12800HZ,		/**< 12800Hz */
	ANALOG_SENSOR_SAMPLING_FREQUENCY_25600HZ		/**< 25600Hz */
}ANALOG_SENSOR_SAMPLING_FREQUENCY;

/**
 * @brief センサーバッファの状態を示す列挙型
 */
typedef enum
{
	ANALOG_SENSOR_BUFFER_STATE_IS_VALID 	= 0,	/**< バッファにデータが集まっており、使用可能*/
	ANALOG_SENSOR_BUFFER_STATE_IS_NOT_FULL 	= 1,	/**< バッファにデータが集まっていない　*/
	ANALOG_SENSOR_BUFFER_STATE_IS_OVERFLOW 	= 2,	/**< オーバーフローしている*/
}ANALOG_SENSOR_BUFFER_STATE;

/**
 * @brief センサーバッファのサイズ
 */
#define ANALOG_SENSOR_BUFFER_SIZE			(128)

/**
 * @brief アナログセンサー用ADCの初期化を行う。
 *
 * @param frequency ADCのサンプリング周波数
 */
void AnalogSensorInit(ANALOG_SENSOR_SAMPLING_FREQUENCY frequency);

/**
 * @brief アナログセンサー用ADCの初期化を行う。
 *
 * @note サンプリング周波数は25,600Hz
 */
void AnalogSensorDefaultInit(void);

/**
 * @brief センサー入力を開始する。
 */
void AnalogSensorStart(void);

/**
 * @brief センサー入力を停止する。
 */
void AnalogSensorStop(void);

/**
 * @brief センサーバッファを使用開始する。
 *
 * @param sensorBuffer センサーバッファを格納するポインタ。バッファのデータが使用可能ならそのバッファのポインタが入る。
 * @return ANALOG_SENSOR_BUFFER_STATE
 */
ANALOG_SENSOR_BUFFER_STATE AnalogSensorStartUsingSensorBuffer(int16_t** sensorBuffer);

/**
 * @brief センサーバッファを使用終了する。
 *
 * @return ANALOG_SENSOR_BUFFER_STATE
 * @note オーバーフローに注意する。
 */
ANALOG_SENSOR_BUFFER_STATE AnalogSensorStopUsingSensorBuffer(void);
#endif // ANALOG_SENSOR_H__
