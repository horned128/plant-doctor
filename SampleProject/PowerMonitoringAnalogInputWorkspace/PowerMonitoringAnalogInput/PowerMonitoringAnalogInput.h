/*****************************************************************************
 * File: PowerMonitoringAnalogInput.h
 * Title: 電源電圧監視アナログ入力を使用する。
 * LastUpdated: 2025.05.30
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PowerMonitoringAnalogInput.h
 * @brief 電源電圧監視アナログ入力を使用する。
 */

#ifndef POWER_MONITORING_ANALOG_INPUT_H__
#define	POWER_MONITORING_ANALOG_INPUT_H__

/**
 * @brief 電源電圧監視アナログ入力機能の初期化をする。
 */
void PowerMonitoringAnalogInputInit(void);

/**
 * @brief 現在の電源電圧値を更新し、取得する。
 *
 * @return float 更新済みの電源電圧値
 */
float PowerMonitoringAnalogInputGetVoltageValue(void);

/**
 * @brief 現在の電源電圧値を取得する。
 *
 * @return float 現在の電源電圧値
 */
float PowerMonitoringAnalogInputCheckCurrentVoltageValue(void);

#endif //POWER_MONITORING_ANALOG_INPUT_H_
