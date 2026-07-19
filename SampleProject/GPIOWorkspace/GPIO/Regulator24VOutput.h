/*****************************************************************************
 * File: Regulator24VOutput.h
 * Title: 24Vレギュレータを制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Regulator24VOutput.h
 * @brief 24Vレギュレータを制御する。
 */
#ifndef REGULATOR_24V_OUTPUT_H__
#define	REGULATOR_24V_OUTPUT_H__
#include "Output.h"

/**
 * @brief 24Vレギュレータ制御の初期化をする。
 */
void Regulator24VOutputInit(void);

/**
 * @brief 24Vレギュレータを使用する。
 */
void Regulator24VOutputOn(void);

/**
 * @brief 24Vレギュレータの使用を中止する。
 */
void Regulator24VOutputOff(void);

/**
 * @brief 24Vレギュレータの使用状態を確認する。
 *
 * @return OUTPUT_STATUS
 */
OUTPUT_STATUS Regulator24VOutputGetOutputStatus(void);
#endif //REGULATOR_24V_OUTPUT_H__
