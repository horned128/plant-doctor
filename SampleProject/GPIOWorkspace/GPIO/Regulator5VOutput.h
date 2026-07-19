/*****************************************************************************
 * File: Regulator5VOutput.h
 * Title: 5Vレギュレータを制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Regulator5VOutput.h
 * @brief 5Vレギュレータを制御する。
 */
#ifndef REGULATOR_5V_OUTPUT_H__
#define	REGULATOR_5V_OUTPUT_H__
#include "Output.h"

/**
 * @brief 5Vレギュレータ制御の初期化をする。
 */
void Regulator5VOutputInit(void);

/**
 * @brief 5Vレギュレータを使用する。
 */
void Regulator5VOutputOn(void);

/**
 * @brief 5Vレギュレータの使用を中止する。
 */
void Regulator5VOutputOff(void);

/**
 * @brief 5Vレギュレータの使用状態を確認する。
 *
 * @return OUTPUT_STATUS
 */
OUTPUT_STATUS Regulator5VOutputGetOutputStatus(void);
#endif //REGULATOR_5V_OUTPUT_H__
