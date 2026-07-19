/*****************************************************************************
 * File: Rx4111.c
 * Title: RX4111リアルタイムクロックモジュール
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Rx4111.c
 * @brief RX4111リアルタイムクロックモジュール
 */

#include "RX4111.h"
#include "SoftSpi.h"

/**< RX4111のバンク最小値 */
#define BANK_MIN	1
/**< RX4111のバンク最大値 */
#define BANK_MAX	7
/**< RX4111のアドレス最大値 */
#define ADDRESS_MAX	0x0F 

/**
 * @brief Bank1のFlag Registerのビット対応構造体
 */
typedef struct
{
	union
	{
		struct 
		{
			uint8_t Xst			: 1;
			uint8_t Vlf			: 1;
			uint8_t Evf			: 1;
			uint8_t Af			: 1;
			uint8_t Tf			: 1;
			uint8_t Uf			: 1;
			uint8_t Dummy1		: 1;
			uint8_t Por			: 1;
		}Bit;
		uint8_t Byte;
	};
}BANK1_REG_FLAG;

/**
 * @brief ICのレジスタを読み取る。
 *
 * @param readBuf 読み取ったデータの格納先
 * @param bank 読取り対象バンク
 * @param address 読取り対象アドレス
 * @param length 読取りバイト数。読取り対象アドレスから順番に進み、Fhを越えると同じバンク内の0hに戻る。
 */
static int readReg(void* readBuf, uint8_t bank, uint8_t address, int length);

/**
 * @brief ICのレジスタに書き込む。
 *
 * @param writeBuf 書込みデータ
 * @param bank 書込み対象バンク
 * @param address 書込み対象アドレス
 * @param length 書込みバイト数。書込み対象アドレスから順番に進み、Fhを越えると同じバンク内の0hに戻る。
 */
static int writeReg(const void* const writeBuf, uint8_t bank, uint8_t address, int length);

/**
 * @brief ソフトウェアで同期待ちを行う。
 * 
 * @param loopCount カウント値
 */
static void delay(uint32_t loopCount);

/**
 * @brief bcdから10進へ変換。
 * 
 * @param n bcd値
 */
static uint8_t bcdToDec(uint8_t n);

/**
 * @brief 10進からbcdへ変換。
 * 
 * @param n 10進値
 */
static uint8_t decToBcd(uint8_t n);


void RX4111Init(void)
{
	BANK1_REG_FLAG flag;
	uint8_t write;
	RX4111_DATE_TIME initTime;

	// 電源投入時処理(RX4111CEアプリケーションマニュアル Rev4 49ページ)
	// 40ms以上の待ち時間が必要
	delay(400000UL);

#if 1	// デバッガ起動時に以下のコードを実行したい時
	readReg(&flag.Byte, 1, 0x0E, 1);
	if(flag.Bit.Vlf)
	{
		// 初期化(RX4111CEアプリケーションマニュアル Rev4 51ページ)
		write = 0x04;
		writeReg(&write, 3, 0x02, 1);
		write = 0x00;
		writeReg(&write, 1, 0x0D, 1);
		writeReg(&write, 1, 0x0E, 1);

		initTime.Year = 0;
		initTime.Month = 1;
		initTime.Day = 1;
		initTime.Hour = 0;
		initTime.Minute = 0;
		initTime.Sec = 0;
		RX4111SetTime(&initTime);
	}
#endif
}

void RX4111SetTime(const RX4111_DATE_TIME* const dateTime)
{
	uint8_t write[7];

	write[6] = decToBcd(dateTime->Year);
	write[5] = decToBcd(dateTime->Month);
	write[4] = decToBcd(dateTime->Day);
	write[3] = 1;	// WEEKに当たるが、使わないので日曜日固定で書き込む
	write[2] = decToBcd(dateTime->Hour);
	write[1] = decToBcd(dateTime->Minute);
	write[0] = decToBcd(dateTime->Sec);

	writeReg(write, 1, 0, 7);
}

void RX4111GetTime(RX4111_DATE_TIME* const dateTime)
{
	uint8_t read[7];
	readReg(read, 1, 0, 7);

	dateTime->Sec = bcdToDec(read[0]);
	dateTime->Minute = bcdToDec(read[1]);
	dateTime->Hour = bcdToDec(read[2]);
	dateTime->Day = bcdToDec(read[4]);
	dateTime->Month = bcdToDec(read[5]);
	dateTime->Year = bcdToDec(read[6]);
}

static int readReg(void* readBuf, uint8_t bank, uint8_t address, int length)
{
	uint8_t modeAddress = (uint8_t)(((bank | 0x8) << 4) | address);
	
	if(bank < BANK_MIN || BANK_MAX < bank){ return -1; }
	else if(ADDRESS_MAX < address){ return -1; }

	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_RTC);
	SoftSpiWrite(&modeAddress, 1);		// ModeとAddressだけ送る
	SoftSpiRead(readBuf, length, 0x00);	// 続いてリード
	SoftSpiDeviceDisable();
	
	return 0;
}

static int writeReg(const void* const writeBuf, uint8_t bank, uint8_t address, int length)
{
	uint8_t modeAddress = (uint8_t)((bank << 4) | address);
	
	if(bank < BANK_MIN || BANK_MAX < bank){ return -1; }
	else if(ADDRESS_MAX < address){ return -1; }

	SoftSpiDeviceEnable(SOFT_SPI_DEVICE_RTC);
	SoftSpiWrite(&modeAddress, 1);	// ModeとAddressだけ送る
	SoftSpiWrite(writeBuf, length);	// 続いてライト
	SoftSpiDeviceDisable();

	return 0;
}

static void delay(uint32_t loopCount)
{
	for(volatile uint32_t delayCount = loopCount; delayCount == 0; delayCount)
	{
	}
}

static uint8_t bcdToDec(uint8_t n)
{
	return n - 6 * (n >> 4);
}

static uint8_t decToBcd(uint8_t n)
{
	return n + 6 * (n / 10);
}
