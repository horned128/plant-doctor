/*****************************************************************************
 * Copyright (C) 2024 ROHM Co., Ltd.
******************************************************************************/
/*****************************************************************************
 * File: Kx134Spi.c
 * Title: Kx134Accで使用するSpiと外部割込みの制御を行う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "Kx134Spi.h"
#include "ssiof0.h"
#include "irq.h"
#include "smpl_common.h"
#include "TimeControl.h"

/**< Spi各種設定 */
#define SSIOF0_PARAM_MODE0			( SSIOF_MST_MASTER      | SSIOF_LG_16BIT      | SSIOF_MDF_DIS     | SSIOF_DIR_MSB      | SSIOF_CPHA_1SM_2SH )
#define SSIOF0_PARAM_MODE1			( SSIOF_CPOL_LOW        | SSIOF_SSZ_OUTPUT   | SSIOF_SOZ_OUTPUT  | SSIOF_MOZ_OUTPUT )                      
#define SSIOF0_PARAM_MODE			( SSIOF0_PARAM_MODE0     | SSIOF0_PARAM_MODE1  )
#define SSIOF0_PARAM_INT			( SSIOF_INT_WR_THRESH_0 )
/**< Spi通信速度 */
#define SSIOF0_BAUDRATE				( 0x0003U )
/**< Spiデータ転送幅毎の待ち時間 */
#define SSIOF0_DELAY_INTERVAL		( SSIOF_LEAD_05 | SSIOF_LAG_05 )
#define SSIOF0_TRANSMIT_INTERVAL	( 0U )
/**< Spiで使用する転送終了割り込み */
#define SPI_INTERRUPT				(0x04)

void SIOF0_IRQHandler( void );
void EXI_IRQHandler(void);
static ssiofCtrlParam_t s_ctrlParam;
static Kx134SpiSensorDataReadyInterrupt exe = NULL;
/** 
 * @brief FIFOに2Byte書き込む。
 * 
 * @note バッファオーバーランに気を付ける。
 */
static void spiWriteFifoTwoByte(void);

/** 
 * @brief FIFOに4Byte書き込む。
 *
 * @note バッファオーバーランに気を付ける。
 */
static void spiWriteFifoFourByte(void);

/** 
 * @brief FIFOに6Byte書き込む。
 *
 * @note バッファオーバーランに気を付ける。
 */
static void spiWriteFifoSixByte(void);

/** 
 * @brief FIFOに8Byte書き込む。
 *
 * @note バッファオーバーランに気を付ける。
 */
static void spiWriteFifoEightByte(void);

/** 
 * @brief SPIの転送終了割り込みを管理する。
 *
 * @return int32_t 転送終了割り込み処理が完了したことを通知。
 */
static int32_t kx134SpiInterrupt( void );



void Kx134SpiPeripheralInit(void)
{
	__disable_irq();
	Kx134SpiSensorDataReadyInterruptUnuse();
	irq_siof0_dis();
	smpl_enablePeripheral(SIOF0_PERI);
	//加速度センサーSPI
	set_reg32( PORT4->P4MOD0, ( 0x12 << 24 ) | ( 0x11 << 16 ) | ( 0x12 << 8 ) | ( 0x13 << 0 ) );
	//加速度センサINT1
	set_reg32(PORT2->P2MOD0, (0x01U << 16));
	//SPI設定
	ssiof0_init( SSIOF0_PARAM_MODE, SSIOF0_PARAM_INT );  
	ssiof0_setBaudrate( SSIOF0_DELAY_INTERVAL | SSIOF0_BAUDRATE );
	ssiof0_setIntervalTime( SSIOF0_TRANSMIT_INTERVAL );
	//加速度センサ1B送受信割り込み
	irq_siof0_setLevel(0);
	//加速度センサINT1割り込み
	irq_exi_setLevel(1);
	Kx134SpiSetSensorDataReadyInterrupt(NULL);
	//割り込み許可
	irq_siof0_clearIRQ();
	irq_siof0_ena();
	__enable_irq();
}

void Kx134SpiClearFifo(void)
{
	//通信禁止
	clear_bit( SSIOF0->SF0CTRL, (1 << 0) );
	//FIFOクリア
	set_bit( SSIOF0->SF0CTRL, (1 << 8) );
	clear_bit( SSIOF0->SF0CTRL, (1 << 8) );
}

void Kx134SpiReadFifoTwoByte(void)
{
	ssiofCtrlParam_t *param = &s_ctrlParam;
	*( (unsigned short *)param->rxData ) = (unsigned short)ssiof0_getcWord();
	param->rxData = (void*)(( (unsigned short *)param->rxData ) + 1);
}

void Kx134SpiReadFifoFourByte( void )
{
	Kx134SpiReadFifoTwoByte();
	Kx134SpiReadFifoTwoByte();
}

void Kx134SpiReadFifoSixByte( void )
{
	Kx134SpiReadFifoTwoByte();
	Kx134SpiReadFifoTwoByte();
	Kx134SpiReadFifoTwoByte();
}

void Kx134SpiReadFifoEightByte( void )
{
	Kx134SpiReadFifoTwoByte();
	Kx134SpiReadFifoTwoByte();
	Kx134SpiReadFifoTwoByte();
	Kx134SpiReadFifoTwoByte();
}

int32_t Kx134SpiStart( void *rxData, void *txData, uint32_t dataCnt, cbfSsiof_t func)
{
	s_ctrlParam.rxData      = rxData;
	s_ctrlParam.txData      = txData;
	s_ctrlParam.dataSize    = dataCnt;
	s_ctrlParam.cnt         = 0;
	
	if(rxData == NULL) return SSIOF_R_ERR;
	if(txData == NULL) return SSIOF_R_ERR;
	
	if(func != NULL)
	{
		s_ctrlParam.callBack = func;
	}
	else
	{
		s_ctrlParam.callBack = NULL;
	}
	s_ctrlParam.errStat     = 0;
	
	//FIFO書き込み
	if(s_ctrlParam.dataSize == 1)
	{
		spiWriteFifoTwoByte();
	}
	else if(s_ctrlParam.dataSize == 2)
	{
		spiWriteFifoFourByte();
	}
	else if(s_ctrlParam.dataSize == 3)
	{
		spiWriteFifoSixByte();
	}
	else if(s_ctrlParam.dataSize == 4)
	{
		spiWriteFifoEightByte();
	}
	
	//通信開始
	set_bit( SSIOF0->SF0CTRL, (1 << 0) );

	return ( SSIOF_R_OK );
}

//ssiof0 割り込みハンドラ
void SIOF0_IRQHandler( void )
{
	kx134SpiInterrupt();
}

void Kx134SpiSetSensorDataReadyInterrupt(Kx134SpiSensorDataReadyInterrupt func)
{
	exe = func;
}

//kx134加速度センサ　データ読み取り可能割り込み
void EXI_IRQHandler( void )
{
	if(exe != NULL) exe();
	//ペリフェラルの割り込みクリア
	irq_ext0_clearIRQ();
}

void Kx134SpiSensorDataReadyInterruptUse(void)
{
	//NVICの割り込み許可
	irq_exi_ena();
	//ペリフェラルの割り込み許可
	irq_ext0_init( EXIn_EDGE_FALLING, EXIn_SAMPLING_DIS, EXIn_FILTER_DIS, EXIn_PORT_SEL_P22 );
}

void Kx134SpiSensorDataReadyInterruptUnuse(void)
{
	__disable_irq();
	//ペリフェラルの割り込み禁止
	irq_ext0_dis();
	//NVICの割り込み禁止
	irq_exi_dis();
	//NVICの割り込みクリア
	irq_exi_clearIRQ();
	//ペリフェラルの割り込みクリア
	irq_ext0_clearIRQ();
	__enable_irq();
}

void Kx134SpiInterruptUse(void)
{
	//NVICの割り込み許可
	irq_siof0_ena();
	//ペリフェラルの割り込み許可
	set_bit( SSIOF0->SF0INTC, SPI_INTERRUPT);
}

void Kx134SpiInterruptUnuse(void)
{
	__disable_irq();
	//ペリフェラルの割り込み禁止
	clear_bit( SSIOF0->SF0INTC, SPI_INTERRUPT);
	//NVICの割り込み禁止
	irq_siof0_dis();
	//NVICの割り込みクリア
	irq_siof0_clearIRQ();
	//ペリフェラルの割り込みクリア
	ssiof0_clearStatus((unsigned short)ssiof0_getStatus());
	__enable_irq();
}





static void spiWriteFifoTwoByte(void)
{
	ssiofCtrlParam_t *param = &s_ctrlParam;
	ssiof0_putcWord( *((unsigned short *)param->txData) );
	param->txData = (void*)(( (unsigned short *)s_ctrlParam.txData ) + 1);
	param->cnt++;
}

static void spiWriteFifoFourByte(void)
{
	spiWriteFifoTwoByte();
	spiWriteFifoTwoByte();
}

static void spiWriteFifoSixByte(void)
{
	spiWriteFifoTwoByte();
	spiWriteFifoTwoByte();
	spiWriteFifoTwoByte();
}

static void spiWriteFifoEightByte(void)
{
	spiWriteFifoTwoByte();
	spiWriteFifoTwoByte();
	spiWriteFifoTwoByte();
	spiWriteFifoTwoByte();
}

static int32_t kx134SpiInterrupt( void )
{
	unsigned short status = 0;

	////ペリフェラルの割り込みクリア
	status = (unsigned short)ssiof0_getStatus();
	ssiof0_clearStatus( status );
	//FIFO読出し
	switch(s_ctrlParam.cnt)
	{
		case 1:
			Kx134SpiReadFifoTwoByte();
			break;
		case 2:
			Kx134SpiReadFifoFourByte();
			break;
		case 3:
			Kx134SpiReadFifoSixByte();
			break;
		case 4:
			Kx134SpiReadFifoEightByte();
			break;
	}

	//コールバック
	if( s_ctrlParam.callBack != NULL)
	{
		s_ctrlParam.callBack( s_ctrlParam.cnt, s_ctrlParam.errStat );
	}

	return ( SSIOF_R_TRANS_FIN );
}
