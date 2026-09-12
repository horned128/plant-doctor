/** =================================================================*
 * @file   SensorPlausibility.c
 * @brief  センサー妥当性・整合性判定
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "SensorPlausibility.h"                             /* 判定API */
#include "PlantDoctorConfig.h"                              /* 設定マクロ */
#include <string.h>                                         /* memset */

/** =================================================================*
 * @brief  判定状態初期化
 * @param[out] state 初期化対象状態
 * ================================================================= */
void SensorPlausibility_Reset(SENSOR_PLAUSIBILITY_STATE *state) {
    if (state != 0) {
        (void)memset(state, 0, sizeof(*state));
    }
}

/** =================================================================*
 * @brief  妥当性・整合性判定更新
 * @param[in,out] state 判定状態
 * @param[in]     input センサー入力値と通信成否
 * ================================================================= */
void SensorPlausibility_Evaluate(SENSOR_PLAUSIBILITY_STATE *state, const SENSOR_PLAUSIBILITY_INPUT *input) {
    if ((state == 0) || (input == 0)) {
        return;
    }

    /* 1. 土壌水分判定（通信 -> 範囲 -> 整合性/急変） */
    if (!input->soilMoistureValid) {
        state->report.soilHealth = SENSOR_HEALTH_NO_COMMUNICATION;
    } else if (input->soilMoistureRaw > 4095U) {
        state->report.soilHealth = SENSOR_HEALTH_OUT_OF_RANGE;
    } else {
        bool stepAbnormal = false;
        if (state->hasLastSoilRaw) {
            int32_t diff = (int32_t)input->soilMoistureRaw - (int32_t)state->lastSoilRaw;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff > (int32_t)PLANT_DOCTOR_SOIL_STEP_MAX_RAW) {
                stepAbnormal = true;
            }
        }
        state->lastSoilRaw = input->soilMoistureRaw;
        state->hasLastSoilRaw = true;

        if (stepAbnormal) {
            state->report.soilHealth = SENSOR_HEALTH_OUT_OF_RANGE;
        } else {
            state->report.soilHealth = SENSOR_HEALTH_OK;
        }
    }

    /* 2. 葉温判定（通信 -> 範囲 -> 葉温-気温差継続異常） */
    if (!input->leafTemperatureValid) {
        state->report.leafHealth = SENSOR_HEALTH_NO_COMMUNICATION;
        state->tempDeltaInconsistentCount = 0U;
    } else if ((input->leafTemperatureCentiC < PLANT_DOCTOR_LEAF_TEMP_MIN_CENTIC) ||
               (input->leafTemperatureCentiC > PLANT_DOCTOR_LEAF_TEMP_MAX_CENTIC)) {
        state->report.leafHealth = SENSOR_HEALTH_OUT_OF_RANGE;
        state->tempDeltaInconsistentCount = 0U;
    } else if (input->airTemperatureValid) {
        int32_t delta = (int32_t)input->leafTemperatureCentiC - (int32_t)input->airTemperatureCentiC;
        if (delta < 0) {
            delta = -delta;
        }
        if (delta > (int32_t)PLANT_DOCTOR_TEMP_DELTA_MAX_CENTIC) {
            if (state->tempDeltaInconsistentCount < PLANT_DOCTOR_PLAUSIBILITY_PERSIST) {
                ++state->tempDeltaInconsistentCount;
            }
            if (state->tempDeltaInconsistentCount >= PLANT_DOCTOR_PLAUSIBILITY_PERSIST) {
                state->report.leafHealth = SENSOR_HEALTH_INCONSISTENT;
            } else {
                state->report.leafHealth = SENSOR_HEALTH_OK;
            }
        } else {
            state->tempDeltaInconsistentCount = 0U;
            state->report.leafHealth = SENSOR_HEALTH_OK;
        }
    } else {
        state->tempDeltaInconsistentCount = 0U;
        state->report.leafHealth = SENSOR_HEALTH_OK;
    }

    /* 3. 気温・湿度判定（通信 -> 範囲） */
    if (!input->airTemperatureValid || !input->relativeHumidityValid) {
        state->report.airHumHealth = SENSOR_HEALTH_NO_COMMUNICATION;
    } else if ((input->airTemperatureCentiC < PLANT_DOCTOR_AIR_TEMP_MIN_CENTIC) ||
               (input->airTemperatureCentiC > PLANT_DOCTOR_AIR_TEMP_MAX_CENTIC) ||
               (input->relativeHumidityCentiPercent > PLANT_DOCTOR_HUMIDITY_MAX_CENTIPC)) {
        state->report.airHumHealth = SENSOR_HEALTH_OUT_OF_RANGE;
    } else {
        state->report.airHumHealth = SENSOR_HEALTH_OK;
    }

    /* 4. 照度判定（通信 -> 照度0継続異常: DEMO5） */
    if (!input->illuminanceValid) {
        state->report.luxHealth = SENSOR_HEALTH_NO_COMMUNICATION;
        state->luxZeroCount = 0U;
    } else if (input->illuminanceRaw == 0U) {
        if (state->luxZeroCount < PLANT_DOCTOR_PLAUSIBILITY_PERSIST) {
            ++state->luxZeroCount;
        }
        if (state->luxZeroCount >= PLANT_DOCTOR_PLAUSIBILITY_PERSIST) {
            state->report.luxHealth = SENSOR_HEALTH_INCONSISTENT;
        } else {
            state->report.luxHealth = SENSOR_HEALTH_OK;
        }
    } else {
        state->luxZeroCount = 0U;
        state->report.luxHealth = SENSOR_HEALTH_OK;
    }
}

/** =================================================================*
 * @brief  最新の判定結果取得
 * @param[in]  state  判定状態
 * @param[out] report 結果出力先
 * ================================================================= */
void SensorPlausibility_GetReport(const SENSOR_PLAUSIBILITY_STATE *state, SENSOR_HEALTH_REPORT *report) {
    if ((state != 0) && (report != 0)) {
        *report = state->report;
    }
}
