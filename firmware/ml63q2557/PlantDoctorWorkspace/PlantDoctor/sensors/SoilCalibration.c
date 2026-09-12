/** =================================================================*
 * @file   SoilCalibration.c
 * @brief  土壌水分校正変換
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "SoilCalibration.h"                /* 土壌水分校正変換API */
#include "PlantDoctorConfig.h"              /* アプリケーション設定 */
#include <stddef.h>                         /* NULL定義 */

/** =================================================================*
 * @brief  校正値の妥当性判定
 * @param[in] dry 乾燥時ADC生値
 * @param[in] wet 湿潤時ADC生値
 * @return 妥当な校正値の場合true、不当な場合false
 * ================================================================= */
bool SoilCalibration_IsValid(uint16_t dry, uint16_t wet) {
    if (wet <= dry) {
        return false;
    }
    if ((uint16_t)(wet - dry) < PLANT_DOCTOR_SOIL_CAL_MIN_DIFF) {
        return false;
    }
    return true;
}

/** =================================================================*
 * @brief  ADC生値を0〜1000の相対湿り度（‰）へ変換
 * @details 水分率(%)ではなく、鉢・土ごとの相対的な湿り度を表す千分率(‰)。
 * @param[in] raw 現在のADC生値
 * @param[in] dry 乾燥時ADC生値
 * @param[in] wet 湿潤時ADC生値
 * @return 相対湿り度（0〜1000‰）
 * ================================================================= */
int16_t SoilCalibration_ToPermille(uint16_t raw, uint16_t dry, uint16_t wet) {
    int32_t span;
    int32_t offset;
    int32_t permille;

    if (!SoilCalibration_IsValid(dry, wet)) {
        return 0;
    }

    if (raw <= dry) {
        return 0;
    }
    if (raw >= wet) {
        return 1000;
    }

    span = (int32_t)(wet - dry);
    offset = (int32_t)(raw - dry);
    permille = (offset * 1000L) / span;

    if (permille < 0L) {
        return 0;
    }
    if (permille > 1000L) {
        return 1000;
    }

    return (int16_t)permille;
}

static uint16_t s_calDry = PLANT_DOCTOR_SOIL_CAL_DEFAULT_DRY;
static uint16_t s_calWet = PLANT_DOCTOR_SOIL_CAL_DEFAULT_WET;

/** =================================================================*
 * @brief  現在の校正値取得
 * @param[out] dry 乾燥時ADC生値出力先
 * @param[out] wet 湿潤時ADC生値出力先
 * ================================================================= */
void SoilCalibration_Get(uint16_t *dry, uint16_t *wet) {
    if (dry != NULL) {
        *dry = s_calDry;
    }
    if (wet != NULL) {
        *wet = s_calWet;
    }
}

/** =================================================================*
 * @brief  校正値設定
 * @param[in] dry 乾燥時ADC生値
 * @param[in] wet 湿潤時ADC生値
 * @return 妥当な値で設定成功時true
 * ================================================================= */
bool SoilCalibration_Set(uint16_t dry, uint16_t wet) {
    if (!SoilCalibration_IsValid(dry, wet)) {
        return false;
    }
    s_calDry = dry;
    s_calWet = wet;
    return true;
}
