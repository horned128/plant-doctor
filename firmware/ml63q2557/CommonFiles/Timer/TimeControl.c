/*****************************************************************************
 * File: TimeControl.c
 * Title: Timer1を制御して時間待ちを行う。
 * LastUpdated: 2025.06.02
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file TimeControl.c
 * @brief Timer1を制御して時間待ちを行う。
 */

#include "TimeControl.h"
#include "timer0_1.h"
#include "wdt.h"
#include "irq.h"
#include "smpl_common.h"

/**< 閾値 */
#define TIMER_MS_MIN						(1)
#define TIMER_MS_MAX						(1000)

/**< LSCLKの1/1000 */
#define LSCLK_MS							(32.768)

/**< TIMER設定値(ms) */
#define TIMER_DATA_REGISTER_MS(interval)	((interval * LSCLK_MS) - 1)

/**< レジスタ設定 */
#define TIMER_PARAM_MODE_MS					( TM_CS_LSCLK | TM_DIV1 | TM_MODE_16BIT | TM_OST_ONESHOT )

volatile static bool timeControlFlg; 


/**
 * @brief 引数が正しいかチェック。
 * 
 * @param xms 1 ~ 1000
 * @return bool 引数が範囲内なら有効を返す。
 */
inline static bool isMsValid(uint16_t xms);

/**
 * @brief 割り込み処理。
 */
static void timeControlInterrupt(void);

void TM1_IRQHandler( void );




void TimeControlInit(void)
{
	__disable_irq();
	
	smpl_enablePeripheral(TM0_PERI | TM1_PERI);
	irq_tm1_dis();
	irq_tm1_clearIRQ();
	irq_tm1_ena();
	
	__enable_irq();
}

bool TimeControlDelayMs(uint16_t xms)
{
	if(!isMsValid(xms)) return false;
	timer1_init( TIMER_PARAM_MODE_MS );
	timer1_setCnt( (uint16_t)TIMER_DATA_REGISTER_MS(xms) );
	timeControlFlg = false;
	timer1_start();
	
	while(timeControlFlg == false)
	{
		wdt_clear();
	}
	return true;
}

void TM1_IRQHandler( void )
{
	timeControlInterrupt();
}





inline static bool isMsValid(uint16_t xms)
{
	if( (TIMER_MS_MIN <= xms) && ( xms <= TIMER_MS_MAX ) ) return true;
	else return false;
}

static void timeControlInterrupt(void)
{
	timeControlFlg = true;
}
