/*****************************************************************************
 * Copyright (C) 2024 ROHM Co., Ltd.
******************************************************************************/
/*****************************************************************************
 * File: LcdI2cf0.c
 * Title: Lcdで使用するI2Cを制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file LcdI2cf0.c
 * @brief Lcdで使用するI2Cを制御する。
 */

#include "mcu.h"
#include "rdwr_reg.h"
#include "LcdI2cf0.h"
#include <stdio.h>
#include <stdbool.h>

#define I2F_COMMUNICATION_END		(0)		/**< 通信完了 */
#define I2F_TRANS_SLAVE_ADDRESS		(1)		/**< スレーブアドレス送信中 */
#define I2F_TRANS_ADDRESS			(2)		/**< 拡張スレーブアドレス送受信中 */
#define I2F_WRITE_DATA				(3)		/**< データ送信中 */
#define I2F_READ_DATA				(4)		/**< データ受信中 */

#define I2C_WRITE_MODE				(0)		/**< 送信モード */
#define I2C_ACK						(0)		/**< ACK */
/**
 * @brief データを送信する。
 *
 * @param data
 */
#define i2cf0Putc( data )		write_reg32( I2CF0->I2F0DR, data )

/**
 * @brief スタートコンディションを発行する。
 */
#define i2cf0TrigStart()		set_bit( I2CF0->I2F0CTL, (1 << 5) )

/**
 * @brief ストップコンディションを発行する。
 */
#define i2cf0TrigStop()			clear_bit( I2CF0->I2F0CTL, (1 << 5) )

/**
 * @brief nackを確認する。
 *
 * @return 0でACK、1でNACKを返す。
 */
#define i2cf0CheckRXAK()		get_bit( I2CF0->I2F0SR, (1 << 0) )

/**
 * @brief 割り込み要求をクリアする。
 */
#define i2cf0ClearCF_IF()	do{                                   \
															clear_bit( I2CF0->I2F0SR, ((1 << 7) | (1 << 1)) );    \
							}while (0)

/**
 * @brief I2Cマスターのノーマルモードで使用するパラメータ
 */
typedef struct {
	uint8_t Mode;			/**< 送信モード(0:write, 1:read) */
	uint8_t ErrStat;		/**< エラーステータス */
	uint8_t Status;			/**< I2C通信の状態 */
	uint8_t *Data;			/**< 送信バッファのポインタ */
	uint32_t DataSize;		/**< 送信バッファのサイズ */
	uint32_t Cnt;			/**< 送信データ数 */
	cbfI2f_t CallBack;		/**< 送信完了時に処理される関数 */
} I2fCtrlParam_t;
volatile static I2fCtrlParam_t ctrlParam;

/**
 * I2C通信中の割込み処理を行う。
 *
 * @return	I2F_R_TRANS_FIN		I2C通信完了したことを通知。
 *			I2F_R_TRANS_CONT_OK	I2C通信中であることを通知。
 */
static int32_t i2cf0Continue( void );

void I2CF0_IRQHandler( void );


void LcdI2cf0InitNormalMode(uint8_t mode, uint8_t rate)
{
	//通信モード
	set_reg32( I2CF0->I2F0CTL, ((mode & 0x03) | (1 << 7)) );
	
	//通信速度
	write_reg32( I2CF0->I2F0BC, rate );
	
	//バッファモード不使用
	clear_bit( I2CF0->I2F0MOD, (1 << 0) );
	
	//DR_LD不使用
	clear_bit( I2CF0->I2F0CTL, (1 << 12) );
	
	//STP不使用
	clear_bit( I2CF0->I2F0CTL, (1 << 11) );
	
	//MCF使用
	set_bit( I2CF0->I2F0CTL, (1 << 9) );
	
	//MAAS使用
	clear_bit( I2CF0->I2F0CTL, (1 << 6) );
}

int32_t LcdI2cf0Write( uint8_t slaveAddr, uint8_t *buf, uint16_t size, cbfI2f_t func )
{
	ctrlParam.Mode = I2C_WRITE_MODE;    
	ctrlParam.Data = buf;
	ctrlParam.DataSize = size;
	ctrlParam.Cnt = 0;
	ctrlParam.CallBack = func;
	ctrlParam.ErrStat = 0;
	ctrlParam.Status = 0;

	//スレーブアドレス送信状態へ。
	ctrlParam.Status = I2F_TRANS_SLAVE_ADDRESS;
	
	//I2Cバスが開放されているか確認。
	while ( get_bit(I2CF0->I2F0SR, (1 << 5)) != 0 ) { }
	
	//I2Cマスターを送信モードへ
	set_bit( I2CF0->I2F0CTL, (1 << 4));
	
	//スレーブアドレスセット。
	write_reg32( I2CF0->I2F0DR, slaveAddr );

	i2cf0TrigStart();
	return ( I2F_R_OK );
}

void I2CF0_IRQHandler( void )
{
	int32_t  state;
	state = i2cf0Continue();
	if (state == I2F_R_TRANS_FIN){
		if( ctrlParam.CallBack != NULL ) ctrlParam.CallBack( ctrlParam.Cnt, ctrlParam.ErrStat );
	}
}



static int32_t i2cf0Continue( void )
{
	switch( ctrlParam.Status )
	{
		case I2F_TRANS_SLAVE_ADDRESS:
			i2cf0ClearCF_IF();		
			if( i2cf0CheckRXAK() != I2C_ACK )
			{
				ctrlParam.Status = I2F_COMMUNICATION_END;
				ctrlParam.ErrStat = I2F_ERR_ACR;
				i2cf0TrigStop();
				return ( I2F_R_TRANS_FIN );
			}
			
			if( ctrlParam.Mode == I2C_WRITE_MODE )
			{
				ctrlParam.Status = I2F_WRITE_DATA;
				i2cf0Putc( (uint8_t)*ctrlParam.Data );
				ctrlParam.Data++;
				ctrlParam.Cnt++;
				return(I2F_R_TRANS_CONT_OK);
			}
			break;
			
		case I2F_WRITE_DATA:
			i2cf0ClearCF_IF();		
			if( i2cf0CheckRXAK() != I2C_ACK )
			{
				ctrlParam.Status = I2F_COMMUNICATION_END;
				ctrlParam.ErrStat = I2F_ERR_ACR;
				i2cf0TrigStop();
				return ( I2F_R_TRANS_FIN );
			}
			
			if( ctrlParam.DataSize > ctrlParam.Cnt )
			{
				clear_bit( I2CF0->I2F0CTL, (1 << 3) );
				i2cf0Putc( (uint8_t)*ctrlParam.Data );
				ctrlParam.Data++;
				ctrlParam.Cnt++;
				return ( I2F_R_TRANS_CONT_OK );
			}
			else
			{
				ctrlParam.Status = I2F_COMMUNICATION_END;
				i2cf0TrigStop();
				return ( I2F_R_TRANS_FIN );
			}	
	}
}


