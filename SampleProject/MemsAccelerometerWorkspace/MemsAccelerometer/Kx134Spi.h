/*****************************************************************************
 * Copyright (C) 2024 ROHM Co., Ltd.
******************************************************************************/
/*****************************************************************************
 * File: Kx134Spi.h
 * Title: Kx134Accで使用するSpiとセンサーデータ読み取り可能割り込みの制御を行う。
 * LastUpdated: 2025.05.30
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Kx134Spi.h
 * @brief Kx134Accで使用するSpiとセンサーデータ読み取り可能割り込みの制御を行う。
 */
#ifndef KX134_SPI_H__
#define KX134_SPI_H__

#include "mcu.h"
#include "rdwr_reg.h"
#include "ssiof_common.h"

/**
 * @brief センサーデータ読み取り可能割り込みで使用する関数型
 */
typedef void (*Kx134SpiSensorDataReadyInterrupt) (void);

/**
 * @brief kx134Accで使用するSpiの設定を行う。
 */
void Kx134SpiPeripheralInit(void);

/**
 * @brief SpiのFIFOをクリアする。
 */
void Kx134SpiClearFifo(void);

/**
 * @brief SpiのFIFOから2Byte読む。
 * @note SPI通信を行った後に呼び出すこと。Spi割込み不使用時専用。
 */
void Kx134SpiReadFifoTwoByte(void);

/**
 * @brief SpiのFIFOから4Byte読む。
 * @note SPI通信を行った後に呼び出すこと。Spi割込み不使用時専用。
 */
void Kx134SpiReadFifoFourByte( void );

/**
 * @brief SpiのFIFOから6Byte読む。
 * @note SPI通信を行った後に呼び出すこと。Spi割込み不使用時専用。
 */
void Kx134SpiReadFifoSixByte( void );

/**
 * @brief SpiのFIFOから8Byte読む。
 * @note SPI通信を行った後に呼び出すこと。Spi割込み不使用時専用。
 */
void Kx134SpiReadFifoEightByte( void );

/**
 * @brief Spi通信を開始する。
 *
 * @param rxData 受信バッファのポインタ
 * @param txData 送信バッファのポインタ
 * @param dataCnt 送信データサイズ
 * @param func Spi通信完了時のコールバック
 * @return int32_t SSIOF_R_OKを返す。バッファのポインタがNULLだとSSIOF_R_ERRを返す。
 */
int32_t Kx134SpiStart( void *rxData, void *txData, uint32_t dataCnt, cbfSsiof_t func);

/**
 * @brief センサーデータ読み取り可能割り込みを許可する。
 */
void Kx134SpiSensorDataReadyInterruptUse(void);

/**
 * @brief センサーデータ読み取り可能割り込みを禁止する。
 *
 * @note 保留中の割り込みをクリアする。
 */
void Kx134SpiSensorDataReadyInterruptUnuse(void);
	
/**
 * @brief Spi割り込みを許可する。
 */
void Kx134SpiInterruptUse(void);
	
/**
 * @brief Spi割り込みを禁止する。
 *
 * @note 保留中の割り込みをクリアする。
 */
void Kx134SpiInterruptUnuse(void);

/**
 * @brief センサーデータ読み取り可能割り込みで使用する関数を設定する。
 *
 * @param func 使用する関数
 */
void Kx134SpiSetSensorDataReadyInterrupt(Kx134SpiSensorDataReadyInterrupt func);
#endif //KX134_SPI_H__
