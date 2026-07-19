/*****************************************************************************
 * File: Fram.c
 * Title: FRAMモジュールのインターフェース
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Fram.c
 * @brief FRAMモジュールのインターフェース
 * @note 本モジュールの複数バイトの読み書きはCPUのエンディアンで行う。
 */

#include "Fram.h"
#include "SoftSpi.h"
#include "mcu.h"
#include "rdwr_reg.h"

#define SET_PORT_OUTPUT_WP()	( set_bit( PORT3->P3MOD0, (1 << 1U) ) )
#define WP_OFF()				( set_bit( PORT3->P3DO, (1 << 0U) ) )
#define WP_ON()					( clear_bit( PORT3->P3DO, (1 << 0U) ) )

/**
 * @brief ライトイネーブルコマンドを送信する。
 */
static void sendCommandWREN(void);

/**
 * @brief ライトディスエーブルコマンドを送信する。
 */
static void sendCommandWRDI(void);

/**
 * @brief ステータスレジスタが全てアンプロテクトになるようなWRSRコマンドを送信する。
 */
static void sendCommandWrsrUnprotect(void);

/**
 * @brief リードコマンドとアドレスを送信する。
 *
 * @note この関数内ではCSは操作しないのであらかじめCSをEnableする。\n
 *       リードが終わったらDisableする事。
 */
static void sendCommandRead(uint32_t address);

/**
 * @brief ライトコマンドとアドレスを送信する。
 *
 * @note この関数内ではCSは操作しないのであらかじめCSをEnableする。\n
 *       リードが終わったらDisableする事。
 */
static void sendCommandWrite(uint32_t address);



void FramInit(void)
{
	SET_PORT_OUTPUT_WP();
	WP_OFF();
	sendCommandWREN();
	sendCommandWrsrUnprotect();
	sendCommandWRDI();
}

void FramReadByte(uint32_t address, uint8_t* byte)
{
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandRead(address);
	SoftSpiRead(byte, 1, 0);
	SoftSpiDeviceDisable();
}

void FramReadHalfWord(uint32_t address, uint16_t* halfWord)
{
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandRead(address);
	SoftSpiRead(halfWord, 2, 0);
	SoftSpiDeviceDisable();
}

void FramReadWord(uint32_t address, uint32_t* word)
{
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandRead(address);
	SoftSpiRead(word, 4, 0);
	SoftSpiDeviceDisable();
}

void FramReadBlock(uint32_t address, void* dst, int size)
{
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandRead(address);
	SoftSpiRead(dst, size, 0);
	SoftSpiDeviceDisable();
}

void FramWriteByte(uint32_t address, uint8_t byte)
{
	sendCommandWREN();

	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandWrite(address);
	SoftSpiWrite(&byte, 1);
	SoftSpiDeviceDisable();

	sendCommandWRDI();
}

void FramWriteHalfWord(uint32_t address, uint16_t halfWord)
{
	sendCommandWREN();

	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandWrite(address);
	SoftSpiWrite(&halfWord, 2);
	SoftSpiDeviceDisable();

	sendCommandWRDI();
}

void FramWriteWord(uint32_t address, uint32_t word)
{
	sendCommandWREN();
	
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandWrite(address);
	SoftSpiWrite(&word, 4);
	SoftSpiDeviceDisable();

	sendCommandWRDI();
}

void FramWriteBlock(uint32_t address, const void* src, int size)
{
	sendCommandWREN();

	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	sendCommandWrite(address);
	SoftSpiWrite(src, size);
	SoftSpiDeviceDisable();

	sendCommandWRDI();
}






static void sendCommandWREN(void)
{
	const uint8_t COMMAND_WREN = 0x06;
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	SoftSpiWrite(&COMMAND_WREN, 1);
	SoftSpiDeviceDisable();
}

static void sendCommandWRDI(void)
{
	const uint8_t COMMAND_WRDI = 0x04;
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	SoftSpiWrite(&COMMAND_WRDI, 1);
	SoftSpiDeviceDisable();
}

static void sendCommandWrsrUnprotect(void)
{
	const uint8_t COMMAND_WRSR_UNPROTECTED[2] = {0x01, 0x00};
	
	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_FRAM);
	SoftSpiWrite(COMMAND_WRSR_UNPROTECTED, 2);
	SoftSpiDeviceDisable();
}

static void sendCommandRead(uint32_t address)
{
	const uint8_t COMMAND_READ = 0x03;
	uint8_t sendData[4] =
	{
		COMMAND_READ,
		(address >> 16) & 0xFF,
		(address >> 8) & 0xFF,
		address & 0xFF
	};
	
	SoftSpiWrite(sendData, sizeof(sendData));
}

static void sendCommandWrite(uint32_t address)
{
	const uint8_t COMMAND_WRITE = 0x02;
	uint8_t sendData[4] =
	{
		COMMAND_WRITE,
		(address >> 16) & 0xFF,
		(address >> 8) & 0xFF,
		address & 0xFF
	};
	
	SoftSpiWrite(sendData, sizeof(sendData));
}
