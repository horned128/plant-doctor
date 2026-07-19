/*****************************************************************************
 main.c

 Copyright (C) 2024 ROHM Co., Ltd.
 All rights reserved.

 This software is provided "as is" and any expressed or implied
 warranties, including, but not limited to, the implied warranties of
 merchantability and fitness for a particular purpose are disclaimed.
 ROHM shall not be liable for any direct, indirect, consequential or
 incidental damages arising from using or modifying this software.
 You (customer) can modify and use this software in whole or part on
 your own responsibility, only for the purpose of developing the software
 for use with microcontroller manufactured by ROHM.

 History
    2024.07.31 Ver 1.0.0

******************************************************************************/
/*****************************************************************************
 * File: main.c
 * Title: UARTで通信を行うプロジェクト。
 * LastUpdated: 2025.05.29
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file main.c
 * @brief UARTで通信を行うプロジェクト。
 */

#include "main.h"
#include "clock.h"
#include "smpl_common.h"
#include "wdt.h"
#include "irq.h"
#include "SystemPowerControl.h"
#include "uartf1.h"
#include "UartInterrupt.h"

/**
 * @brief UARTF1に対する設定
 *
 * @note 
 * データ長:8
 * ストップビット: 1
 * パリティビット: なし
 */
#define UARTF_PARAM_MODE	( UARTF_LG_8BIT | UARTF_STP_1BIT | UARTF_PT_NON | UARTF_BC_DIS | UARTF_DLAB_RBR_THR | UARTF_RFR_KEEP | UARTF_TFR_KEEP | UARTF_FTL_2BYTE )
#define UARTF_PARAM_CAJ		( UARTF_RMV_ENA | 0x0019U )
/**< ボーレート: 9600bps */
#define UARTF_PARAM_DLR		( 0x012CU )
/**< 受信バッファデータ数 */
#define UARTF_RW_MAX		( 10U )
/**< フラグ 完了かどうか */
#define FLAG_CLR			( 0U ) 
#define FLAG_SET			( 1U )

/**< 受信データ数 */
static uint32_t s_rwSize;
/**< 受信バッファ */
static uint8_t s_rwBuffer[UARTF_RW_MAX];
/**< 通信制御フラグ */
static uint8_t s_flag;
/**< コールバック */
static uint16_t s_cbfErrStat; 

/**
 * @brief UARTF1に対する設定を行う。
 */
static void uartf1Init( void );

/**
 * @brief UART受信コールバック。受信したデータをそのまま送信する。
 *
 * @param size 受信データ数
 * @param errStatus
 */
static void s_procUartfRead( uint32_t size, uint16_t errStatus );

/**
 * @brief UART送信コールバック。
 *
 * @param size 送信データ数
 * @param errStatus
 */
static void s_procUartfWrite( uint32_t size, uint16_t errStatus );

/**
 * @brief 割り込み毎のデータ処理を行う。
 */
static void uartInterruptController( void );

static void systemInit(void);

int main(void)
{
	systemInit();

	s_flag   = FLAG_CLR;
	s_rwSize = UARTF_RW_MAX;
	//受信開始
	uartf1_read( s_rwBuffer, s_rwSize, s_procUartfRead );
	//受信、送信完了まで待機
	while( s_flag == FLAG_CLR ){
		wdt_clear();
	}
	
	//後処理
	uartf1_stopRead();
	uartf1_stopWrite();
	irq_uaf1_dis();
	
	while(1){
		wdt_clear();
	}
}

static void systemInit(void)
{
	__disable_irq();

	wdt_init( WDT_2S );
	wdt_clear();
	smpl_setLsCrystal32Khz();
	smpl_setHsPll48Mhz( CLK_XSPEN_DIS,CLK_HXSPEN_DIS );
	SystemPowerControlInit();
	uartf1Init();
	
	__enable_irq();
}

static void uartf1Init( void )
{
	smpl_enablePeripheral(UAF1_PERI);
	__disable_irq();
	
	irq_uaf1_dis();
	set_reg32(PORT7->P7MOD0, (0x22U << 8U) | (0x21U << 0U)); 
	uartf1_init( (uint16_t)UARTF_PARAM_MODE, (uint16_t)UARTF_PARAM_CAJ, (uint16_t)UARTF_PARAM_DLR );
	UartInterruptSetFunc(uartInterruptController);
	irq_uaf1_clearIRQ();
	irq_uaf1_ena();
	
	__enable_irq();
}

static void s_procUartfRead( uint32_t size, uint16_t errStatus )
{
	s_rwSize = size;
	s_cbfErrStat = errStatus;
	if( size != 0U ) {
		uartf1_write( s_rwBuffer, size, s_procUartfWrite );
	} else {
		s_flag = FLAG_SET;     
	}
}

static void s_procUartfWrite( uint32_t size, uint16_t errStatus )
{
	s_flag  = FLAG_SET;  
	s_rwSize = size;
	s_cbfErrStat = errStatus;
}

static void uartInterruptController( void )
{
	uint32_t intStat;
	intStat = uartf1_getIntCause() & UARTF_IRID_MASK;
	switch( intStat ) {
	case UARTF_IRID_READ_REQ:
	case UARTF_IRID_CHAR_TIMEOUT:
	case UARTF_IRID_DATA_ERR:
		uartf1_continueRead();
		break;
	case UARTF_IRID_WRITE_REQ:
	case UARTF_IRID_TRANS_COMP:
		uartf1_continueWrite( (uint16_t)intStat );
		break;
	default:
		break;
	}
}
