/*****************************************************************************
 * File: Input.h
 * Title: 汎用入力を使う。
 * LastUpdated: 2025.05.30
 * Copyright (C) 2025 DATA TECNO Co., Ltd.
******************************************************************************/

/**
 * @file Input.h
 * @brief 汎用入力を使う。
 */

#ifndef INPUT_H__
#define INPUT_H__
#include <stdint.h>
#include <stdbool.h>
/**
 * @brief 入力のindexを示す列挙型
 */
typedef enum 
{
	INPUT_INDEX_DIP_PSH = 0,							/**< ディップスイッチ、プッシュスイッチそれぞれ4つ */
	INPUT_INDEX_POWER_MONITORING,						/**< 電源スイッチ */
	INPUT_INDEX_PHOTO_COUPLER_INPUT,					/**< フォトカプラ入力2つ */
	INPUT_INDEX_END,									/**< 無効 */
}INPUT_INDEX;

/**
 * @brief ポーリングの結果を示す列挙型
 * 		  イベントは入力値が変化したことを指す。
 */
typedef enum
{
	INPUT_POLLING_RESULT_INVALID_ARGUMENT = 0,			/**< 無効な引数 */
	INPUT_POLLING_RESULT_NORMAL,						/**< 変化なし */
	INPUT_POLLING_RESULT_EVENT_OCCURED,					/**< イベント発生 */
}INPUT_POLLING_RESULT;

/**
 * @brief 入力値を読み取る関数型
 */
typedef uint8_t(*InputGetTargetValue)(void);

/**
 * @brief イベント発生時に処理される関数型
 */
typedef void (*InputFunc)(void);

/**
 * @brief 使いたい入力機能の初期化をする。
 * 
 * @param index 使いたい機能のindex 
 * @param initStatus 入力の初期値
 * @return bool indexが範囲外だと初期化失敗でfalseを返す。
 */
bool InputInit(INPUT_INDEX index, uint8_t initStatus);

/**
 * @brief 入力のポーリングを行う。
 * 
 * @param index 使いたい機能のindex 
 * @param waitingTime 	入力が安定するまで待つ時間。2以上を入力。
						10ms周期でのポーリングで使用を想定。
 * @param getValue 入力値を読み取る関数。この関数を使用するモジュールから設定する。
 * @param func イベント発生時に処理したい関数。この関数を使用するモジュールから設定する。
 * @return INPUT_POLLING_RESULT
 * @note 待ち時間(waitingTime)はポーリング周期に合わせて調整する。
 */
INPUT_POLLING_RESULT InputPolling(INPUT_INDEX index, uint8_t waitingTime, InputGetTargetValue getValue, InputFunc func);

/**
 * @brief システム処理上の入力値を取得する。
 *
 * @note InputGetTargetValueで読み取れるその瞬間の入力値と違い、システムで制御している入力値。
 * @param index 使いたい機能のindex 
 * @return uint8_t
 */
uint8_t InputGetSystemInputValue(INPUT_INDEX index);
#endif //INPUT_H__
