/*****************************************************************************
 * File: PowerKeep.h
 * Title: 電圧保持(POWER_KEEP)を制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PowerKeep.h
 * @brief 電圧保持(POWER_KEEP)を制御する。
 */
#ifndef POWER_KEEP_H__
#define	POWER_KEEP_H__

/**
 * @brief 電圧保持制御機能の初期化をする。
 */
void PowerKeepInit(void);

/**
 * @brief 電圧保持をする。
 */
void PowerKeepOn(void);

/**
 * @brief 電圧保持しない。
 */
void PowerKeepOff(void);
#endif //POWER_KEEP_H__
