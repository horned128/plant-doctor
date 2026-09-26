/** =================================================================*
 * @file   PlantFeature.c
 * @brief  植物特徴量抽出
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "PlantFeature.h"                                   /* 植物特徴量型とAPI */
#include <string.h>                                         /* メモリ初期化関数 */

/** =================================================================*
 * @brief  特徴量状態初期化
 * @param[out] state 初期化対象の特徴量状態構造体
 * ================================================================= */
void PlantFeature_Reset(PLANT_FEATURE_STATE *state) {
    if (state != 0) {
        (void)memset(state, 0, sizeof(*state));
        state->vector.validMask = 0U;
    }
}

/** =================================================================*
 * @brief  1サンプル分の特徴量更新
 * @param[in,out] state 特徴量状態
 * @param[in]     input 1サンプル分のセンサー入力
 * ================================================================= */
void PlantFeature_Update(PLANT_FEATURE_STATE *state, const PLANT_FEATURE_INPUT *input) {
    uint8_t i;
    int32_t soilSum = 0;
    uint8_t soilValidCount = 0U;
    int32_t leafSum = 0;
    uint8_t leafValidCount = 0U;
    int32_t accumSum = 0;

    if ((state == 0) || (input == 0)) {
        return;
    }

    /* 給水経過秒の更新 */
    if (input->wateringOccurred) {
        state->elapsedSinceWateringSeconds = 0U;
    } else if (state->elapsedSinceWateringSeconds < UINT32_MAX) {
        ++state->elapsedSinceWateringSeconds;
    }
    state->vector.elapsedSinceWateringSeconds = state->elapsedSinceWateringSeconds;

    state->vector.validMask = 0U;

    /* 土壌水分生値および短期バッファ登録 */
    if (input->soilMoistureValid) {
        state->vector.soilMoisturePermille = input->soilMoisturePermille;
        state->vector.validMask |= PLANT_FEATURE_VALID_SOIL_PERMILLE;
        state->soilBuffer[state->secondIndex] = (int16_t)input->soilMoisturePermille;
        state->soilValidBuffer[state->secondIndex] = true;
    } else {
        state->soilValidBuffer[state->secondIndex] = false;
    }

    /* 葉温短期バッファ登録 */
    if (input->leafTemperatureValid) {
        state->leafBuffer[state->secondIndex] = input->leafTemperatureCentiC;
        state->leafValidBuffer[state->secondIndex] = true;
    } else {
        state->leafValidBuffer[state->secondIndex] = false;
    }

    /* 葉温－気温差 */
    if (input->leafTemperatureValid && input->airTemperatureValid) {
        state->vector.leafAirTemperatureDelta =
            (int32_t)input->leafTemperatureCentiC - (int32_t)input->airTemperatureCentiC;
        state->vector.validMask |= PLANT_FEATURE_VALID_LEAF_AIR_DELTA;
    }

    /* 湿度 */
    if (input->relativeHumidityValid) {
        state->vector.relativeHumidityCentiPercent = (int32_t)input->relativeHumidityCentiPercent;
        state->vector.validMask |= PLANT_FEATURE_VALID_HUMIDITY;
    }

    /* 照度 */
    if (input->illuminanceValid) {
        state->vector.illuminanceRaw = (int32_t)input->illuminanceRaw;
        state->vector.validMask |= PLANT_FEATURE_VALID_ILLUMINANCE_RAW;
    }

    /* 秒インデックスの更新 */
    state->secondIndex = (uint8_t)((state->secondIndex + 1U) % PLANT_FEATURE_BUFFER_SIZE);
    if (state->secondCount < PLANT_FEATURE_BUFFER_SIZE) {
        ++state->secondCount;
    }

    /* 土壌水分・葉温の短期移動平均算出 */
    for (i = 0U; i < state->secondCount; ++i) {
        if (state->soilValidBuffer[i]) {
            soilSum += state->soilBuffer[i];
            ++soilValidCount;
        }
        if (state->leafValidBuffer[i]) {
            leafSum += state->leafBuffer[i];
            ++leafValidCount;
        }
    }

    if (soilValidCount > 0U) {
        state->vector.soilMoistureMovingAverage = soilSum / (int32_t)soilValidCount;
        state->vector.validMask |= PLANT_FEATURE_VALID_SOIL_MA;
    }

    /* 1分間隔の間引き履歴登録（secondIndexが0へ周回したとき） */
    if (state->secondIndex == 0U) {
        if (soilValidCount > 0U) {
            state->soilMinuteHistory[state->minuteHistoryIndex] =
                (int16_t)(soilSum / (int32_t)soilValidCount);
            state->soilMinuteValid[state->minuteHistoryIndex] = true;
        } else {
            state->soilMinuteValid[state->minuteHistoryIndex] = false;
        }

        if (leafValidCount > 0U) {
            state->leafMinuteHistory[state->minuteHistoryIndex] =
                (int16_t)(leafSum / (int32_t)leafValidCount);
            state->leafMinuteValid[state->minuteHistoryIndex] = true;
        } else {
            state->leafMinuteValid[state->minuteHistoryIndex] = false;
        }

        state->minuteHistoryIndex = (uint8_t)((state->minuteHistoryIndex + 1U) % PLANT_FEATURE_MINUTE_HISTORY_SIZE);
        if (state->minuteHistoryCount < PLANT_FEATURE_MINUTE_HISTORY_SIZE) {
            ++state->minuteHistoryCount;
        }

        /* 照度の時間積算 */
        if (input->illuminanceValid) {
            state->currentHourIlluminanceSum += (int32_t)input->illuminanceRaw;
        }
        ++state->minuteInHour;
        if (state->minuteInHour >= 60U) {
            state->hourlyIlluminance[state->hourHistoryIndex] = state->currentHourIlluminanceSum;
            state->hourHistoryIndex = (uint8_t)((state->hourHistoryIndex + 1U) % PLANT_FEATURE_ACCUM_HOURS);
            if (state->hourHistoryCount < PLANT_FEATURE_ACCUM_HOURS) {
                ++state->hourHistoryCount;
            }
            state->currentHourIlluminanceSum = 0;
            state->minuteInHour = 0U;
            if (state->hourHistoryCount >= PLANT_FEATURE_ACCUM_HOURS) {
                state->hasAccumulatedIlluminance = true;
            }
        }
    }

    /* 1時間あたり変化速度の算出（2分以上の履歴がある場合に算出） */
    if (state->minuteHistoryCount >= 2U) {
        uint8_t oldestIdx;
        uint8_t newestIdx;
        int32_t elapsedMin;

        if (state->minuteHistoryCount < PLANT_FEATURE_MINUTE_HISTORY_SIZE) {
            oldestIdx = 0U;
            newestIdx = (uint8_t)(state->minuteHistoryCount - 1U);
            elapsedMin = (int32_t)(state->minuteHistoryCount - 1U);
        } else {
            oldestIdx = state->minuteHistoryIndex;
            newestIdx = (uint8_t)((state->minuteHistoryIndex + PLANT_FEATURE_MINUTE_HISTORY_SIZE - 1U)
                                  % PLANT_FEATURE_MINUTE_HISTORY_SIZE);
            elapsedMin = (int32_t)(PLANT_FEATURE_MINUTE_HISTORY_SIZE - 1U);
        }

        if (elapsedMin > 0) {
            /* 土壌水分低下速度 = (過去値 - 最新値) * 60 / 分数 */
            if (state->soilMinuteValid[oldestIdx] && state->soilMinuteValid[newestIdx]) {
                int32_t oldestSoil = (int32_t)state->soilMinuteHistory[oldestIdx];
                int32_t newestSoil = (int32_t)state->soilMinuteHistory[newestIdx];
                state->vector.soilMoistureRatePerHour = ((oldestSoil - newestSoil) * 60L) / elapsedMin;
                state->vector.validMask |= PLANT_FEATURE_VALID_SOIL_RATE;
            }

            /* 葉温上昇速度 = (最新値 - 過去値) * 60 / 分数 */
            if (state->leafMinuteValid[oldestIdx] && state->leafMinuteValid[newestIdx]) {
                int32_t oldestLeaf = (int32_t)state->leafMinuteHistory[oldestIdx];
                int32_t newestLeaf = (int32_t)state->leafMinuteHistory[newestIdx];
                state->vector.leafTemperatureRatePerHour = ((newestLeaf - oldestLeaf) * 60L) / elapsedMin;
                state->vector.validMask |= PLANT_FEATURE_VALID_LEAF_RATE;
            }
        }
    }

    /* 積算照度（完了した時間枠 + 現在進行中の時間枠） */
    for (i = 0U; i < state->hourHistoryCount; ++i) {
        accumSum += state->hourlyIlluminance[i];
    }
    accumSum += state->currentHourIlluminanceSum;
    state->vector.illuminanceAccumulated = accumSum;
    if (state->hasAccumulatedIlluminance) {
        state->vector.validMask |= PLANT_FEATURE_VALID_ILLUMINANCE_ACCUM;
    }
}

/** =================================================================*
 * @brief  特徴量ベクトル取得
 * @param[in]  state  特徴量状態
 * @param[out] vector 最新特徴量ベクトルの出力先
 * ================================================================= */
void PlantFeature_GetVector(const PLANT_FEATURE_STATE *state, PLANT_FEATURE_VECTOR *vector) {
    if ((state != 0) && (vector != 0)) {
        *vector = state->vector;
    }
}
