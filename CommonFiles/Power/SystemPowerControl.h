/*****************************************************************************
 * File: SystemPowerControl.h
 * Title: 電源関係(POWER_KEEP、POWSW_CHK)の制御。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file SystemPowerControl.h
 * @brief 電源関係(POWER_KEEP、POWSW_CHK)の制御。
 */
#ifndef SYSTEM_POWER_CONTROL_H__
#define	SYSTEM_POWER_CONTROL_H__

/**
 * @brief 電源制御を初期化し、電源電圧を保持する。
 */
void SystemPowerControlInit(void);

/**
 * @brief 電源電圧の保持を終了。
 */
void SystemPowerControlFin(void);
#endif //SYSTEM_POWER_CONTROL_H__
