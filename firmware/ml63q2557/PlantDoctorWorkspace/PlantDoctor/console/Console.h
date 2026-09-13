/** =================================================================*
 * @file   Console.h
 * @brief  UARTコンソールコマンド解釈API (P2-4)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantLogRecord.h"                 /* ログレコード型定義 */
#include "SensorManager.h"                  /* センサースナップショット型定義 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief コンソールサービス依存性注入構造体
 */
typedef struct {
    uint16_t (*getRecordCount)(void);                       /* 保持レコード数取得 */
    bool (*readRecord)(uint16_t index, PLANT_LOG_RECORD *record); /* index番目のレコード取得 */
    bool (*eraseLog)(void);                                 /* 記録全消去 */
    uint32_t (*getUnixSeconds)(void);                       /* 現在のUNIX秒取得 */
    bool (*setUnixSeconds)(uint32_t unixSeconds);           /* 時刻設定 */
    bool (*getSnapshot)(PLANT_SENSOR_SNAPSHOT *snapshot);    /* 最新スナップショット取得 */
    bool (*getDiagnosis)(uint8_t *stressScore, uint8_t *status, const char **soilTrend);
    bool (*getCalibration)(uint16_t *dry, uint16_t *wet);    /* 校正値取得 */
    bool (*setCalibration)(uint16_t dry, uint16_t wet);      /* 校正値設定 */
    uint8_t (*getMaxPendingTicks)(void);                     /* 最大未処理tick数取得 */
    bool (*setDemoMode)(bool enable);                        /* デモモード切替（P8-1） */
    uint8_t (*triggerWatering)(void);                        /* 給水要求 (0:OK, 1:EMPTY, 2:COOLDOWN, 3:ERR) */
    bool (*isPumpOn)(void);                                  /* ポンプ動作状態取得 */
} CONSOLE_SERVICES;

void Console_Init(const CONSOLE_SERVICES *services);
void Console_PutRxChar(char ch);
void Console_Process10Ms(void);
bool Console_GetTxChar(char *ch);
bool Console_HasTxData(void);
bool Console_IsDumping(void);
void Console_Reset(void);

/* 同期コマンド実行API (ホスト単体テスト用) */
bool Console_ExecuteCommand(const char *cmd, char *response, uint16_t maxLen);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_H */
