/*****************************************************************************
 * File: Sw.h
 * Title: Sw(プッシュスイッチ/ディップスイッチ)を使う。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Sw.h
 * @brief Sw(プッシュスイッチ/ディップスイッチ)を使う。\n
          プッシュスイッチは回路図のSW2を1、SW3を2、SW4を3、SW5を4とする。\n
          ディップスイッチは回路図のSW1の1を1、2を2、3を3、4を4とする。
 */
#ifndef SW_H__
#define	SW_H__
#include "Input.h"
#include <stdbool.h>
/**
 * @brief スイッチ入力機能の初期化をする。
 */
void SwInit(void);
 
/**
 * @brief スイッチ入力のポーリングを行う。
 * 
 * @return INPUT_POLLING_RESULT
 */
INPUT_POLLING_RESULT SwPolling(void);

/**
 * @brief プッシュスイッチ1入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsPsw1Entered(void);

/**
 * @brief プッシュスイッチ2入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsPsw2Entered(void);

/**
 * @brief プッシュスイッチ3入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsPsw3Entered(void);

/**
 * @brief プッシュスイッチ4入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsPsw4Entered(void);

/**
 * @brief プッシュスイッチ1入力を次の押下まで入力停止する。
 */
void SwDisablePsw1UntilNextPress(void);

/**
 * @brief プッシュスイッチ2入力を次の押下まで入力停止する。
 */
void SwDisablePsw2UntilNextPress(void);

/**
 * @brief プッシュスイッチ3入力を次の押下まで入力停止する。
 */
void SwDisablePsw3UntilNextPress(void);

/**
 * @brief プッシュスイッチ4入力を次の押下まで入力停止する。
 */
void SwDisablePsw4UntilNextPress(void);

/**
 * @brief プッシュスイッチ入力を次の押下まで入力停止する。
 */
void SwDisableAllPswUntilNextPress(void);

/**
 * @brief ディップスイッチ1入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsDsw1Entered(void);

/**
 * @brief ディップスイッチ2入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsDsw2Entered(void);

/**
 * @brief ディップスイッチ3入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsDsw3Entered(void);

/**
 * @brief ディップスイッチ4入力を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool SwIsDsw4Entered(void);
#endif //SW_H__
