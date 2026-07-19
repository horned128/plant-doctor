/*****************************************************************************
 * Copyright (C) 2024 ROHM Co., Ltd.
******************************************************************************/
/*****************************************************************************
 * File: LcdI2cf0.h
 * Title: Lcdで使用するI2Cを制御する。
 * LastUpdated: 2025.05.23
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file LcdI2cf0.h
 * @brief Lcdで使用するI2Cを制御する。
 */
#ifndef LCD_I2CF0_H__
#define LCD_I2CF0_H__

#include "rdwr_reg.h"
#include "i2cf_common.h"

/**
 * @brief I2Cをノーマルモードで初期化する。
 *
 * @param mode I2Cのモード。標準またはファストを選択。
 * @param rate 速度
 */
void LcdI2cf0InitNormalMode(uint8_t mode, uint8_t rate);

/**
 * @brief ノーマルモードでI2Cの送信を行う。
 *
 * @param slaveAddr	スレーブアドレス
 * @param buf	送信バッファのポインタ
 * @param size	送信バッファのサイズ
 * @param func	送信完了時に処理される関数。正常終了またはnackを確認した時に処理。
 * @return　I2F_R_OK(=0)　I2C通信が開始されたことを通知。
 */
int32_t LcdI2cf0Write(uint8_t slaveAddr, uint8_t *buf, uint16_t size, cbfI2f_t func);
#endif //LCD_I2CF0_H__
