/*****************************************************************************
 * File: PhotoCouplerInput.h
 * Title: フォトカプラ入力を取り扱う。
 * LastUpdated: 2025.06.02
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file PhotoCouplerInput.h
 * @brief フォトカプラ入力を取り扱う。\n
 *		  回路図のISOIN0をフォトカプラ入力0とする。\n
 *		  回路図のISOIN1をフォトカプラ入力1とする。
 */
#ifndef PHOTO_COUPLER_INPUT_H__
#define	PHOTO_COUPLER_INPUT_H__
#include "Input.h"
#include <stdbool.h>

/**
 * @brief フォトカプラ入力機能の初期化をする。
 */
void PhotoCouplerInputInit(void);

/**
 * @brief フォトカプラ入力のポーリングを行う。
 * 
 * @return INPUT_POLLING_RESULT
 */
INPUT_POLLING_RESULT PhotoCouplerInputPolling(void);

/**
 * @brief フォトカプラ入力0を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool PhotoCouplerInput0IsEntered(void);

/**
 * @brief フォトカプラ入力1を確認する。
 * 
 * @return bool 入力でtrueを返す。
 */
bool PhotoCouplerInput1IsEntered(void);
#endif //PHOTO_COUPLER_INPUT_H__
