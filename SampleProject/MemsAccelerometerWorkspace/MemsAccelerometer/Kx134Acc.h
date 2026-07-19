/*****************************************************************************
 * File: Kx134Acc.h
 * Title: MEMS加速度センサーKx134-1211を制御するモジュール
 * LastUpdated: 2025.05.30
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Kx134Acc.h
 * @brief MEMS加速度センサーKx134-1211を制御するモジュール
 * @details センサーのデータはバッファで管理する。
 */
#ifndef KX134_ACC_H__
#define KX134_ACC_H__
#include <stdint.h>

/**
 * @brief MEMS加速度センサーの軸の列挙型
 */
typedef enum
{
	KX134_ACC_AXIS_X = 0,						/**< X軸 */
	KX134_ACC_AXIS_Y,							/**< Y軸 */
	KX134_ACC_AXIS_Z,							/**< Z軸 */
}KX134_ACC_AXIS;

/**
 * @brief MEMS加速度センサーのサンプリング周波数の列挙型
 */
typedef enum
{
	KX134_ACC_SAMPLING_FREQUENCY_100HZ = 7,		/**< 100Hz */
	KX134_ACC_SAMPLING_FREQUENCY_200HZ,			/**< 200Hz */
	KX134_ACC_SAMPLING_FREQUENCY_400HZ,			/**< 400Hz */
	KX134_ACC_SAMPLING_FREQUENCY_800HZ,			/**< 800Hz */
	KX134_ACC_SAMPLING_FREQUENCY_1600HZ,		/**< 1600Hz */
	KX134_ACC_SAMPLING_FREQUENCY_3200HZ,		/**< 3200Hz */
	KX134_ACC_SAMPLING_FREQUENCY_6400HZ,		/**< 6400Hz */
	KX134_ACC_SAMPLING_FREQUENCY_12800HZ,		/**< 12800Hz */
	KX134_ACC_SAMPLING_FREQUENCY_25600HZ		/**< 25600Hz */
}KX134_ACC_SAMPLING_FREQUENCY;


/**
 * @brief MEMS加速度センサーのLPFの列挙型
 */
typedef enum
{
	KX134_ACC_LPF_ODR_9 = 0,					/**< ODR/9 */
	KX134_ACC_LPF_ODR_2,						/**< ODR/2 */
}KX134_ACC_LPF;


/**
 * @brief センサーバッファの状態を示す列挙型
 */
typedef enum
{
	KX134_ACC_BUFFER_STATE_IS_VALID 	= 0,	/**< バッファにデータが集まっており、使用可能*/
	KX134_ACC_BUFFER_STATE_IS_NOT_FULL 	= 1,	/**< バッファにデータが集まっていない　*/
	KX134_ACC_BUFFER_STATE_IS_OVERFLOW 	= 2,	/**< オーバーフローしている*/
}KX134_ACC_BUFFER_STATE;

/**
 * @brief センサーバッファのサイズ
 */
#define KX134_ACC_BUFFER_SIZE					(128)

/**
 * @brief MEMS加速度センサーを初期化。
 *
 * @param type MEMS加速度センサーの軸
 * @param frequency MEMS加速度センサーのサンプリング周波数
 * @param filter MEMS加速度センサーのLPF
 */
void Kx134AccInit(
	KX134_ACC_AXIS type, 
	KX134_ACC_SAMPLING_FREQUENCY frequency, 
	KX134_ACC_LPF filter);

/**
 * @brief MEMS加速度センサーを初期化。
 *
 * @note センサーの設定は以下の通り。\n
 * - 軸: Z\n
 * - サンプリング周波数: 6,400Hz\n
 * - LPF: ODR/2\n
 */
void Kx134AccDefaultInit(void);

/**
 * @brief センサー入力を開始する。
 */
void Kx134AccStart(void);

/**
 * @brief センサー入力を停止する。
 */
void Kx134AccStop(void);

/**
 * @brief センサーバッファを使用開始する。
 *
 * @param sensorBuffer センサーバッファを格納するポインタ。バッファのデータが使用可能ならそのバッファのポインタが入る。
 * @return KX134_ACC_BUFFER_STATE
 */
KX134_ACC_BUFFER_STATE Kx134AccStartUsingSensorBuffer(int16_t** sensorBuffer);

/**
 * @brief センサーバッファを使用終了する。
 *
 * @return KX134_ACC_BUFFER_STATE
 * @note オーバーフローに注意する。
 */
KX134_ACC_BUFFER_STATE Kx134AccStopUsingSensorBuffer(void);
#endif //KX134_ACC_H__
