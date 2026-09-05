/*****************************************************************************
 * File: TimeControl.h
 * Title: Timer1を制御して時間待ちを行う。
 * LastUpdated: 2025.06.02
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file TimeControl.h
 * @brief Timer1を制御して時間待ちを行う。
 */
 
#ifndef TIME_CONTROL_H__
#define TIME_CONTROL_H__
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief TimeControlの初期化を行う。
 */
void TimeControlInit(void);

/**
 * @brief ms単位での同期待ちを行う。
 *
 * @param xms 1~1000
 * @return bool 引数が適切な範囲でないとfalseを返す。
 */
bool TimeControlDelayMs(uint16_t xms);
#endif //TIME_CONTROL_H__
