/** =================================================================*
 * @file   PlantDiagnosis.c
 * @brief  植物診断・原因候補推定実装 (P4-2)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "PlantDiagnosis.h"
#include <stddef.h>

/** =================================================================*
 * @brief  診断状態を初期化
 * @param  state 診断状態
 * ================================================================= */
void PlantDiagnosis_Reset(PLANT_DIAGNOSIS_STATE *state) {
    if (state == NULL) {
        return;
    }
    state->status = PLANT_STATUS_HEALTHY;
    state->soilTrend = SOIL_TREND_UNKNOWN;
    state->failedSensor = DIAGNOSIS_FAILED_SENSOR_NONE;
}

/** =================================================================*
 * @brief  特徴量・健全性から原因候補および土壌傾向を推定
 * @param  state  診断状態
 * @param  input  診断入力データ
 * @param  config 診断しきい値設定
 * ================================================================= */
void PlantDiagnosis_Update(PLANT_DIAGNOSIS_STATE *state,
                           const PLANT_DIAGNOSIS_INPUT *input,
                           const PLANT_DIAGNOSIS_CONFIG *config) {
    if ((state == NULL) || (input == NULL) || (config == NULL)) {
        return;
    }

    /* -------------------------------------------------------------
     * 1. 状態（Status）の判定
     * 優先順位:
     *   SENSOR_ERROR > WATERING_FAILED > WATERING > DRY_STRESS >
     *   SOIL_DEGRADATION > HEAT_STRESS > ROOT_UPTAKE > LOW_LIGHT >
     *   HEALTHY
     * ------------------------------------------------------------- */
    if (input->soilHealth != SENSOR_HEALTH_OK) {
        state->status = PLANT_STATUS_SENSOR_ERROR;
        state->failedSensor = DIAGNOSIS_FAILED_SENSOR_SOIL;
    } else if (input->leafHealth != SENSOR_HEALTH_OK) {
        state->status = PLANT_STATUS_SENSOR_ERROR;
        state->failedSensor = DIAGNOSIS_FAILED_SENSOR_LEAF;
    } else if (input->airHumHealth != SENSOR_HEALTH_OK) {
        state->status = PLANT_STATUS_SENSOR_ERROR;
        state->failedSensor = DIAGNOSIS_FAILED_SENSOR_AIR_HUM;
    } else if (input->luxHealth != SENSOR_HEALTH_OK) {
        state->status = PLANT_STATUS_SENSOR_ERROR;
        state->failedSensor = DIAGNOSIS_FAILED_SENSOR_LUX;
    } else {
        state->failedSensor = DIAGNOSIS_FAILED_SENSOR_NONE;

        if (input->isWateringFailed) {
            state->status = PLANT_STATUS_WATERING_FAILED;
        } else if (input->isWateringActive) {
            state->status = PLANT_STATUS_WATERING;
        } else if (((input->features.validMask & PLANT_FEATURE_VALID_SOIL_PERMILLE) != 0U) &&
                   (input->features.soilMoisturePermille < config->dryThresholdPermille)) {
            state->status = PLANT_STATUS_DRY_STRESS;
        } else if (input->isSoilDegraded) {
            state->status = PLANT_STATUS_SOIL_DEGRADATION;
        } else if (((input->features.validMask & (PLANT_FEATURE_VALID_LEAF_AIR_DELTA | PLANT_FEATURE_VALID_LEAF_RATE)) ==
                    (PLANT_FEATURE_VALID_LEAF_AIR_DELTA | PLANT_FEATURE_VALID_LEAF_RATE)) &&
                   (input->features.leafAirTemperatureDelta >= config->heatStressTempDelta) &&
                   (input->features.leafTemperatureRatePerHour >= config->heatStressRatePerHour)) {
            state->status = PLANT_STATUS_HEAT_STRESS;
        } else if (((input->features.validMask & (PLANT_FEATURE_VALID_SOIL_PERMILLE | PLANT_FEATURE_VALID_LEAF_AIR_DELTA)) ==
                    (PLANT_FEATURE_VALID_SOIL_PERMILLE | PLANT_FEATURE_VALID_LEAF_AIR_DELTA)) &&
                   (input->features.soilMoisturePermille >= config->rootUptakeSoilMinPermille) &&
                   (input->features.leafAirTemperatureDelta >= config->rootUptakeTempDelta) &&
                   input->wateringDidNotCoolLeaf) {
            state->status = PLANT_STATUS_ROOT_UPTAKE;
        } else if (((input->features.validMask & PLANT_FEATURE_VALID_ILLUMINANCE_ACCUM) != 0U) &&
                   (input->features.illuminanceAccumulated < config->lowLightAccumulatedThreshold)) {
            state->status = PLANT_STATUS_LOW_LIGHT;
        } else {
            state->status = PLANT_STATUS_HEALTHY;
        }
    }

    /* -------------------------------------------------------------
     * 2. 土壌傾向（Soil Trend）の判定
     * ------------------------------------------------------------- */
    if (input->isSoilDegraded) {
        state->soilTrend = SOIL_TREND_DEGRADATION;
    } else if ((input->features.validMask & PLANT_FEATURE_VALID_SOIL_RATE) == 0U) {
        state->soilTrend = SOIL_TREND_UNKNOWN;
    } else if (input->features.soilMoistureRatePerHour <= config->soilDryRatePerHour) {
        state->soilTrend = SOIL_TREND_DRY;
    } else if (input->features.soilMoistureRatePerHour >= config->soilWetRatePerHour) {
        state->soilTrend = SOIL_TREND_WET;
    } else {
        state->soilTrend = SOIL_TREND_STABLE;
    }
}

/** =================================================================*
 * @brief  現在の診断結果（Status）を取得
 * @param  state 診断状態
 * @return 植物診断結果ステータス
 * ================================================================= */
PLANT_STATUS PlantDiagnosis_GetStatus(const PLANT_DIAGNOSIS_STATE *state) {
    if (state == NULL) {
        return PLANT_STATUS_HEALTHY;
    }
    return state->status;
}

/** =================================================================*
 * @brief  現在の土壌傾向（Soil Trend）を取得
 * @param  state 診断状態
 * @return 土壌水分変化傾向
 * ================================================================= */
SOIL_TREND PlantDiagnosis_GetSoilTrend(const PLANT_DIAGNOSIS_STATE *state) {
    if (state == NULL) {
        return SOIL_TREND_UNKNOWN;
    }
    return state->soilTrend;
}

/** =================================================================*
 * @brief  センサー異常時の異常発生グループを取得
 * @param  state 診断状態
 * @return 異常センサーグループ
 * ================================================================= */
DIAGNOSIS_FAILED_SENSOR PlantDiagnosis_GetFailedSensor(const PLANT_DIAGNOSIS_STATE *state) {
    if (state == NULL) {
        return DIAGNOSIS_FAILED_SENSOR_NONE;
    }
    return state->failedSensor;
}
