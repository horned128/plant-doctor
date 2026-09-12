/** =================================================================*
 * @file   SoilCalibration.h
 * @brief  土壌水分校正変換API
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#ifndef SOIL_CALIBRATION_H
#define SOIL_CALIBRATION_H
#include <stdbool.h>                        /* 標準Cの真偽値型 */
#include <stdint.h>                         /* 標準Cの固定幅整数型 */

int16_t SoilCalibration_ToPermille(uint16_t raw, uint16_t dry, uint16_t wet); /* 生値を0〜1000‰の相対湿り度へ変換 */
bool SoilCalibration_IsValid(uint16_t dry, uint16_t wet);   /* 校正値妥当性判定 */
void SoilCalibration_Get(uint16_t *dry, uint16_t *wet);     /* 現在の校正値取得 */
bool SoilCalibration_Set(uint16_t dry, uint16_t wet);       /* 校正値設定 */

#endif /* SOIL_CALIBRATION_H */
