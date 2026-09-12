/** =================================================================*
 * @file   WateringResponse.c
 * @brief  水やり後自己診断・土壌劣化推定実装 (P6-1, P6-2)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "WateringResponse.h"
#include <stddef.h>

/** =================================================================*
 * @brief  給水応答状態を初期化
 * @param  state 応答状態
 * ================================================================= */
void WateringResponse_Reset(WATERING_RESPONSE_STATE *state) {
    if (state == NULL) {
        return;
    }
    state->status = WATERING_RESPONSE_IDLE;
    state->startTick = 0U;
    state->delayedGainPermille = 0;
    state->tempDropCentic = 0;
    state->didNotCoolLeaf = false;
    state->isSoilDegraded = false;
    state->consecutiveDegradeCycles = 0U;
    state->delayedMeasured = false;
    state->tempMeasured = false;

    state->wateringEvent.startTimeSeconds = 0xFFFFFFFFU;
    state->wateringEvent.onDurationTicks = 0U;
    state->wateringEvent.soilMoistureRawBefore = 0U;
    state->wateringEvent.soilMoisturePermilleBefore = 0;
    state->wateringEvent.leafAirDeltaBefore = 0;
    state->wateringEvent.stopReason = (uint8_t)PUMP_STOP_REASON_MANUAL;
    state->wateringEvent.tankLiquidAtStart = false;
    state->wateringEvent.automatic = false;
}

/** =================================================================*
 * @brief  新規給水完了イベントを通知して測定を開始
 * @param  state       応答状態
 * @param  event       給水イベント情報
 * @param  currentTick 現在のTick
 * ================================================================= */
void WateringResponse_NotifyWatering(WATERING_RESPONSE_STATE *state,
                                     const PUMP_WATERING_EVENT *event,
                                     uint32_t currentTick) {
    if ((state == NULL) || (event == NULL)) {
        return;
    }
    state->wateringEvent = *event;
    state->startTick = currentTick;
    state->status = WATERING_RESPONSE_MEASURING;
    state->delayedMeasured = false;
    state->tempMeasured = false;
    state->delayedGainPermille = 0;
    state->tempDropCentic = 0;
    state->didNotCoolLeaf = false;
}

/** =================================================================*
 * @brief  給水応答および土壌劣化を更新
 * @param  state   応答状態
 * @param  input   測定入力データ
 * @param  config  判定設定
 * ================================================================= */
void WateringResponse_Update(WATERING_RESPONSE_STATE *state,
                             const WATERING_RESPONSE_INPUT *input,
                             const WATERING_RESPONSE_CONFIG *config) {
    uint32_t elapsedSeconds;
    uint32_t elapsedTicks;
    bool isDelayedTime;
    bool isTempTime;
    bool isTimedOut;

    if ((state == NULL) || (input == NULL) || (config == NULL)) {
        return;
    }

    if (state->status != WATERING_RESPONSE_MEASURING) {
        return;
    }

    elapsedTicks = input->currentTick - state->startTick;

    if ((input->nowSeconds != 0xFFFFFFFFU) && (state->wateringEvent.startTimeSeconds != 0xFFFFFFFFU)) {
        if (input->nowSeconds >= state->wateringEvent.startTimeSeconds) {
            elapsedSeconds = input->nowSeconds - state->wateringEvent.startTimeSeconds;
        } else {
            elapsedSeconds = elapsedTicks / 100U;
        }
    } else {
        elapsedSeconds = elapsedTicks / 100U;
    }

    isDelayedTime = (elapsedSeconds >= config->delayedSeconds) || (elapsedTicks >= config->delayedTicks);
    isTempTime = (elapsedSeconds >= config->tempDeltaSeconds) || (elapsedTicks >= config->tempDeltaTicks);
    isTimedOut = (elapsedSeconds >= config->timeoutSeconds) || (elapsedTicks >= config->timeoutTicks);

    /* -------------------------------------------------------------
     * 1. 浸透後土壌水分回復判定 (delayedSeconds 経過時)
     * ------------------------------------------------------------- */
    if (!state->delayedMeasured) {
        if (isDelayedTime) {
            if (input->soilMoistureValid) {
                state->delayedGainPermille = (int16_t)(input->soilMoisturePermille - state->wateringEvent.soilMoisturePermilleBefore);
                state->delayedMeasured = true;

                /* 水分が十分に回復しなかった場合 (給水失敗) */
                if (state->delayedGainPermille < config->minSoilRecoveryPermille) {
                    state->status = WATERING_RESPONSE_FAILED;
                    return;
                }

                /* 水分回復あり: 土壌劣化サイクル判定 (P6-2) */
                if (state->delayedGainPermille < config->degradeMinGainPermille) {
                    if (state->consecutiveDegradeCycles < UINT8_MAX) {
                        ++state->consecutiveDegradeCycles;
                    }
                    if (state->consecutiveDegradeCycles >= config->degradeRequiredCycles) {
                        state->isSoilDegraded = true;
                    }
                } else {
                    state->consecutiveDegradeCycles = 0U;
                    state->isSoilDegraded = false;
                }
            } else if (isTimedOut) {
                /* 土壌水分が無効なままタイムアウトした場合はFAILEDにせずIDLEへ戻す */
                state->status = WATERING_RESPONSE_IDLE;
                return;
            }
        }
        return;
    }

    /* -------------------------------------------------------------
     * 2. 蒸散回復・葉温低下判定 (tempDeltaSeconds 経過時)
     * ------------------------------------------------------------- */
    if (!state->tempMeasured) {
        if (isTempTime) {
            if (input->leafAirDeltaValid) {
                state->tempDropCentic = (int16_t)(state->wateringEvent.leafAirDeltaBefore - input->leafAirDeltaCentic);
                state->tempMeasured = true;

                if (state->tempDropCentic >= config->minTempDropCentic) {
                    state->didNotCoolLeaf = false;
                } else {
                    /* 土壌は湿ったが葉温が下がらない -> 根の吸水不良候補フラグ */
                    state->didNotCoolLeaf = true;
                }
                state->status = WATERING_RESPONSE_OK;
                return;
            } else if (isTimedOut) {
                /* 葉温センサが無効なままタイムアウト: 土壌回復済みなのでOKとし、葉温低下なしと記録 */
                state->didNotCoolLeaf = true;
                state->status = WATERING_RESPONSE_OK;
                return;
            }
        }
    }
}

/** =================================================================*
 * @brief  現在の給水応答評価結果を取得
 * @param  state 応答状態
 * @return 応答結果 (IDLE / MEASURING / OK / FAILED)
 * ================================================================= */
WATERING_RESPONSE WateringResponse_GetResult(const WATERING_RESPONSE_STATE *state) {
    if (state == NULL) {
        return WATERING_RESPONSE_IDLE;
    }
    return state->status;
}

/** =================================================================*
 * @brief  給水後も葉温が下がらなかったか（根の吸水不良候補）を取得
 * @param  state 応答状態
 * @return 葉温が下がらなかった場合はtrue
 * ================================================================= */
bool WateringResponse_DidNotCoolLeaf(const WATERING_RESPONSE_STATE *state) {
    if (state == NULL) {
        return false;
    }
    return state->didNotCoolLeaf;
}

/** =================================================================*
 * @brief  土壌劣化が推定されているかを取得 (P6-2)
 * @param  state 応答状態
 * @return 劣化推定時はtrue
 * ================================================================= */
bool WateringResponse_IsSoilDegraded(const WATERING_RESPONSE_STATE *state) {
    if (state == NULL) {
        return false;
    }
    return state->isSoilDegraded;
}

/** =================================================================*
 * @brief  FAILED状態を手動解除してIDLEへ戻す
 * @param  state 応答状態
 * ================================================================= */
void WateringResponse_ClearFailure(WATERING_RESPONSE_STATE *state) {
    if (state == NULL) {
        return;
    }
    if (state->status == WATERING_RESPONSE_FAILED) {
        state->status = WATERING_RESPONSE_IDLE;
    }
}
