/** =================================================================*
 * @file   WateringPolicy.c
 * @brief  自律水やり判定ポリシー実装 (P5-5)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "WateringPolicy.h"
#include <stddef.h>

#define SECONDS_PER_DAY                     (86400UL)
#define TICKS_PER_DAY                       (8640000UL)

/** =================================================================*
 * @brief  自律水やり状態を初期化
 * @param  state ポリシー状態
 * ================================================================= */
void WateringPolicy_Reset(WATERING_POLICY_STATE *state) {
    if (state == NULL) {
        return;
    }
    state->lastWateringSeconds = 0xFFFFFFFFU;
    state->lastWateringTick = 0U;
    state->dailyWateringCount = 0U;
    state->lastDayIndex = 0xFFFFFFFFU;
}

/** =================================================================*
 * @brief  7つの条件に基づき自律給水の要否を判定
 * @param  state   ポリシー状態（日付更新などの副作用あり）
 * @param  input   判定入力
 * @param  config  判定しきい値設定
 * @return 判定結果 (HOLD / REQUEST / BLOCKED)
 * ================================================================= */
WATERING_DECISION WateringPolicy_Evaluate(WATERING_POLICY_STATE *state,
                                         const WATERING_POLICY_INPUT *input,
                                         const WATERING_POLICY_CONFIG *config) {
    bool isDry;
    bool intervalElapsed;

    if ((state == NULL) || (input == NULL) || (config == NULL)) {
        return WATERING_DECISION_HOLD;
    }

    /* -------------------------------------------------------------
     * 条件1: 診断結果が DRY_STRESS かつ 土壌水分 < 乾燥しきい値
     * ------------------------------------------------------------- */
    isDry = (input->plantStatus == PLANT_STATUS_DRY_STRESS) &&
            (input->soilMoisturePermille < config->dryThresholdPermille);

    if (!isDry) {
        return WATERING_DECISION_HOLD;
    }

    /* 乾燥状態を検出したため、以降の安全・制約条件で不成立なら BLOCKED */

    /* 自律給水全体の有効フラグチェック */
    if (!config->autoWateringEnabled) {
        return WATERING_DECISION_BLOCKED;
    }

    /* -------------------------------------------------------------
     * 条件2: 土壌水分センサが SENSOR_HEALTH_OK
     * ------------------------------------------------------------- */
    if (input->soilSensorHealth != SENSOR_HEALTH_OK) {
        return WATERING_DECISION_BLOCKED;
    }

    /* -------------------------------------------------------------
     * 条件3: タンク液面が検出されている
     * ------------------------------------------------------------- */
    if (!input->tankLiquidDetected) {
        return WATERING_DECISION_BLOCKED;
    }

    /* -------------------------------------------------------------
     * 条件4: 前回給水から最小間隔が経過している
     * ------------------------------------------------------------- */
    if ((input->nowSeconds != 0xFFFFFFFFU) && (state->lastWateringSeconds != 0xFFFFFFFFU)) {
        if (input->nowSeconds >= state->lastWateringSeconds) {
            intervalElapsed = ((input->nowSeconds - state->lastWateringSeconds) >= config->minIntervalSeconds);
        } else {
            intervalElapsed = true; /* 時計逆戻り等のフェイルセーフ */
        }
    } else {
        /* 時刻未同期時はTick差分で判定 */
        if ((state->lastWateringSeconds == 0xFFFFFFFFU) && (state->lastWateringTick == 0U)) {
            intervalElapsed = true; /* 初回給水 */
        } else {
            intervalElapsed = ((input->currentTick - state->lastWateringTick) >= config->minIntervalTicks);
        }
    }

    if (!intervalElapsed) {
        return WATERING_DECISION_BLOCKED;
    }

    /* -------------------------------------------------------------
     * 条件5: 直近の給水応答が WATERING_RESPONSE_FAILED でない
     * ------------------------------------------------------------- */
    if (input->lastWateringResponse == WATERING_RESPONSE_FAILED) {
        return WATERING_DECISION_BLOCKED;
    }

    /* -------------------------------------------------------------
     * 条件6: 1日あたりの給水回数が上限未満
     * ------------------------------------------------------------- */
    if (input->nowSeconds != 0xFFFFFFFFU) {
        uint32_t currentDay = input->nowSeconds / SECONDS_PER_DAY;
        if (state->lastDayIndex != currentDay) {
            state->dailyWateringCount = 0U;
            state->lastDayIndex = currentDay;
        }
    } else {
        /* 未同期時は直近給水から24時間相当のTickが経過していればリセット */
        if ((input->currentTick - state->lastWateringTick) >= TICKS_PER_DAY) {
            state->dailyWateringCount = 0U;
        }
    }

    if (state->dailyWateringCount >= config->maxDailyWateringCount) {
        return WATERING_DECISION_BLOCKED;
    }

    /* -------------------------------------------------------------
     * 条件7: アプリケーションが APP_STATE_MONITOR
     * ------------------------------------------------------------- */
    if (!input->isMonitoring) {
        return WATERING_DECISION_BLOCKED;
    }

    return WATERING_DECISION_REQUEST;
}

/** =================================================================*
 * @brief  給水実行をポリシー状態に通知
 * @param  state        ポリシー状態
 * @param  nowSeconds   給水完了時UNIX秒
 * @param  currentTick  給水完了時Tick
 * ================================================================= */
void WateringPolicy_NotifyWateringExecuted(WATERING_POLICY_STATE *state,
                                          uint32_t nowSeconds,
                                          uint32_t currentTick) {
    if (state == NULL) {
        return;
    }

    if (nowSeconds != 0xFFFFFFFFU) {
        uint32_t currentDay = nowSeconds / SECONDS_PER_DAY;
        if (state->lastDayIndex != currentDay) {
            state->dailyWateringCount = 0U;
            state->lastDayIndex = currentDay;
        }
    }

    state->lastWateringSeconds = nowSeconds;
    state->lastWateringTick = currentTick;
    if (state->dailyWateringCount < UINT8_MAX) {
        ++state->dailyWateringCount;
    }
}
