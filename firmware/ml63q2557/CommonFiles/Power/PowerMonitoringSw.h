/*****************************************************************************
 * File: PowerMonitoringSw.h
 * Title: 電源監視用SW(POWSW_CHK)を取り扱う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PowerMonitoringSw.h
 * @brief 電源監視用SW(POWSW_CHK)を取り扱う。
 */

#ifndef POWER_MONITORING_SW_H__
#define	POWER_MONITORING_SW_H__
#include "Input.h"
#include <stdbool.h>
/**
 * @brief 電源監視用SW入力機能の初期化をする。
 */
void PowerMonitoringSwInit(void);

/**
 * @brief 電源監視用SW入力のポーリングを行う。
 * 
 * @return INPUT_POLLING_RESULT
 */
INPUT_POLLING_RESULT PowerMonitoringSwPolling(void);

/**
 * @brief 電源監視用SW入力を確認する。
 * 
 * @return bool 入力ならtrueを返す。
 */
bool PowerMonitoringSwIsPressed(void);
#endif //POWER_MONITORING_SW_H__
