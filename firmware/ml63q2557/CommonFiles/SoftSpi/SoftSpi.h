/*****************************************************************************
 * File: SoftSpi.h
 * Title: ソフトウェアSPIモジュール
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file SoftSpi.h
 * @brief ソフトウェアSPIモジュール
 */

#ifndef SOFT_SPI_H__
#define SOFT_SPI_H__

#include <stdint.h>

/**
 * @brief SoftSpiで扱うデバイスを示す列挙型
 */
typedef enum
{
	SOFT_SPI_DEVICE_RTC = 0,	/**< RTC */
	SOFT_SPI_DEVICE_FRAM,		/**< FRAM */
} SOFT_SPI_DEVICE;

/**
 * @brief ソフトウェアSPIで使用するペリフェラルを初期化する。
 */
void SoftSpiPeripheralInit(void);

/**
 * @brief Spi通信を行うデバイスを指定し、通信開始の準備をする。
 * 
 * @param dev 通信を行うデバイス
 */
void SoftSpiDeviceEnable(SOFT_SPI_DEVICE dev);

/**
 * @brief 全てのデバイスのCSを無効にし、Spi通信を終了する。
 */
void SoftSpiDeviceDisable(void);

/**
 * @brief SPI書込をMSBファーストのSPIモード3(CPOL=1, CPHA=1)で行う。
 * 
 * @note 必ずSoftSpiDeviceEnableにより準備を行った後に実行すること。
 * 
 * @param writeBuf 書込みデータ
 * @param length 書込みデータのバイト数
 */
void SoftSpiWrite(const void* writeBuf, int length);

/**
 * @brief SPI読込をMSBファーストのSPIモード3(CPOL=1, CPHA=1)で行う。
 * 
 * @note 必ずSoftSpiDeviceEnableにより準備を行った後に実行すること。
 * 
 * @param readBuf データの格納先
 * @param length 読込バイト数
 * @param dummyData 読込のため送出する際の値
 */
void SoftSpiRead(void* readBuf, int length, uint8_t dummyData);

/**
 * @brief 全二重通信をMSBファーストのSPIモード3(CPOL=1, CPHA=1)で行う。
 * 
 * @note 必ずSoftSpiDeviceEnableにより準備を行った後に実行すること。
 * 
 * @param writeBuf 書込みデータ
 * @param readBuf 書込み時に同時に読込まれたデータの格納先
 * @param writeBufLength 書込みデータのバイト数
 * @param dummyData 送出するダミーの値
 * @param dummyLength 書込みデータ送信後に送出するダミーのバイト数
 */
void SoftSpiDuplex(
	const void* writeBuf,
	void* readBuf,
	int writeBufLength,
	uint8_t dummyData,
	int dummyLength);

#endif // SOFT_SPI_H__
