/** =================================================================*
 * @file   TimeKeeper.h
 * @brief  時刻管理・日時変換API (P2-5)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef TIME_KEEPER_H
#define TIME_KEEPER_H

#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */
#include "PlantDateTime.h"                  /* 日時構造体型定義 */

#ifdef __cplusplus
extern "C" {
#endif

void TimeKeeper_Init(uint32_t unixSeconds, bool synchronized);
void TimeKeeper_Tick10Ms(void);
uint32_t TimeKeeper_GetUnixSeconds(void);
bool TimeKeeper_IsSynchronized(void);
void TimeKeeper_Resynchronize(uint32_t unixSeconds);

uint32_t TimeKeeper_ToUnixSeconds(const PLANT_DATE_TIME *dateTime);
void TimeKeeper_FromUnixSeconds(uint32_t unixSeconds, PLANT_DATE_TIME *dateTime);

#ifdef __cplusplus
}
#endif

#endif /* TIME_KEEPER_H */
