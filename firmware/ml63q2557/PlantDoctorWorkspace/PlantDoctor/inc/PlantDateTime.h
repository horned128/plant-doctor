/** =================================================================*
 * @file   PlantDateTime.h
 * @brief  日時型定義 (P2-5)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef PLANT_DATE_TIME_H
#define PLANT_DATE_TIME_H

#include <stdint.h>                         /* 標準Cの固定幅整数型 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;                          /* 西暦 (例: 2026) */
    uint8_t month;                          /* 月 (1〜12) */
    uint8_t day;                            /* 日 (1〜31) */
    uint8_t hour;                           /* 時 (0〜23) */
    uint8_t minute;                         /* 分 (0〜59) */
    uint8_t second;                         /* 秒 (0〜59) */
    uint8_t dayOfWeek;                      /* 曜日 (0:日曜〜6:土曜) */
} PLANT_DATE_TIME;

#ifdef __cplusplus
}
#endif

#endif /* PLANT_DATE_TIME_H */
