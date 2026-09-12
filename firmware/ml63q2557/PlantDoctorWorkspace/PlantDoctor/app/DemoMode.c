/** =================================================================*
 * @file   DemoMode.c
 * @brief  デモモード基盤実装 (P8-1)
 * @author Plant Doctor team
 * @date   2026-09
 * ================================================================= */
#include "DemoMode.h"
#include <stddef.h>

/** =================================================================*
 * @brief  デモモード状態初期化
 * @param  state デモモード状態
 * ================================================================= */
void DemoMode_Reset(DEMO_MODE_STATE *state) {
    if (state == NULL) {
        return;
    }
    state->active = false;
    state->scenario = DEMO_SCENARIO_OFF;
    state->sw1HoldTicks = 0U;
    state->sw1LongPressTriggered = false;

    state->offsets.leafTemperatureOffsetCentiC = 0;
    state->offsets.soilMoistureOffsetPermille = 0;
    state->offsets.illuminanceScalePercent = 100;
    state->offsets.forceTankEmpty = false;
    state->offsets.forceSoilSensorError = false;
}

/** =================================================================*
 * @brief  SW1の押下状態を10ms周期で処理（長押しでトグル、短押しでシナリオ切り替え）
 * @param  state                デモモード状態
 * @param  sw1Pressed           SW1押下中フラグ
 * @param  holdThresholdTicks   長押し判定Tick数 (既定300 = 3秒)
 * ================================================================= */
void DemoMode_ProcessSwitch1Tick(DEMO_MODE_STATE *state, bool sw1Pressed, uint16_t holdThresholdTicks) {
    if (state == NULL) {
        return;
    }

    if (sw1Pressed) {
        if (state->sw1HoldTicks < UINT16_MAX) {
            ++state->sw1HoldTicks;
        }
        if ((state->sw1HoldTicks >= holdThresholdTicks) && (!state->sw1LongPressTriggered)) {
            state->sw1LongPressTriggered = true;
            /* 3秒長押し: デモモード有効/無効をトグル */
            if (state->active) {
                DemoMode_SetActive(state, false);
            } else {
                DemoMode_SetActive(state, true);
                DemoMode_SetScenario(state, DEMO_SCENARIO_1_NORMAL);
            }
        }
    } else {
        if (!state->sw1LongPressTriggered && (state->sw1HoldTicks > 0U)) {
            /* 短押し (リリース時): デモモード動作中ならシナリオを順送り */
            if (state->active) {
                DEMO_SCENARIO next = (DEMO_SCENARIO)(state->scenario + 1);
                if (next > DEMO_SCENARIO_5_SENSOR_ERROR) {
                    next = DEMO_SCENARIO_1_NORMAL;
                }
                DemoMode_SetScenario(state, next);
            }
        }
        state->sw1HoldTicks = 0U;
        state->sw1LongPressTriggered = false;
    }
}

/** =================================================================*
 * @brief  デモモード動作中判定
 * @param  state デモモード状態
 * @return 動作中ならtrue
 * ================================================================= */
bool DemoMode_IsActive(const DEMO_MODE_STATE *state) {
    if (state == NULL) {
        return false;
    }
    return state->active;
}

/** =================================================================*
 * @brief  デモモード有効/無効を設定
 * @param  state  デモモード状態
 * @param  active 有効フラグ
 * ================================================================= */
void DemoMode_SetActive(DEMO_MODE_STATE *state, bool active) {
    if (state == NULL) {
        return;
    }
    state->active = active;
    if (!active) {
        DemoMode_SetScenario(state, DEMO_SCENARIO_OFF);
    }
}

/** =================================================================*
 * @brief  デモシナリオを設定しオフセットを反映
 * @param  state    デモモード状態
 * @param  scenario 設定シナリオ
 * ================================================================= */
void DemoMode_SetScenario(DEMO_MODE_STATE *state, DEMO_SCENARIO scenario) {
    if (state == NULL) {
        return;
    }
    state->scenario = scenario;

    /* 全オフセットを既定値へ初期化 */
    state->offsets.leafTemperatureOffsetCentiC = 0;
    state->offsets.soilMoistureOffsetPermille = 0;
    state->offsets.illuminanceScalePercent = 100;
    state->offsets.forceTankEmpty = false;
    state->offsets.forceSoilSensorError = false;

    switch (scenario) {
        case DEMO_SCENARIO_2_HEAT_STRESS:
            state->offsets.leafTemperatureOffsetCentiC = 200; /* +2.00 ℃ (DEMO2) */
            state->offsets.illuminanceScalePercent = 200;     /* 照度2倍 */
            break;

        case DEMO_SCENARIO_3_DRY_STRESS:
            state->offsets.soilMoistureOffsetPermille = -350; /* 土壌水分大幅低下 (DEMO3) */
            break;

        case DEMO_SCENARIO_4_WATERING_FAILED:
            state->offsets.soilMoistureOffsetPermille = -350;
            state->offsets.forceTankEmpty = true;            /* タンク空または給水不達 (DEMO4) */
            break;

        case DEMO_SCENARIO_5_SENSOR_ERROR:
            state->offsets.forceSoilSensorError = true;       /* 土壌センサ異常 (DEMO5) */
            break;

        case DEMO_SCENARIO_OFF:
        case DEMO_SCENARIO_1_NORMAL:
        default:
            /* オフセットなし（実測値そのまま） */
            break;
    }
}

/** =================================================================*
 * @brief  現在のデモシナリオを取得
 * @param  state デモモード状態
 * @return 現在のシナリオ
 * ================================================================= */
DEMO_SCENARIO DemoMode_GetScenario(const DEMO_MODE_STATE *state) {
    if (state == NULL) {
        return DEMO_SCENARIO_OFF;
    }
    return state->scenario;
}

/** =================================================================*
 * @brief  センサ値にデモモードのオフセット・補正を適用
 * @param  state                デモモード状態
 * @param  leafTempCentiC       葉温ポインタ
 * @param  soilMoisturePermille 土壌水分千分率ポインタ
 * @param  illuminanceRaw       照度生値ポインタ
 * @param  tankLiquid           液面検出ポインタ
 * @param  soilHealth           土壌健全性ポインタ
 * ================================================================= */
void DemoMode_ApplyOffsets(const DEMO_MODE_STATE *state,
                           int16_t *leafTempCentiC,
                           int16_t *soilMoisturePermille,
                           int32_t *illuminanceRaw,
                           bool *tankLiquid,
                           SENSOR_HEALTH *soilHealth) {
    if ((state == NULL) || (!state->active)) {
        return;
    }

    if (leafTempCentiC != NULL) {
        *leafTempCentiC = (int16_t)(*leafTempCentiC + state->offsets.leafTemperatureOffsetCentiC);
    }

    if (soilMoisturePermille != NULL) {
        int32_t val = (int32_t)(*soilMoisturePermille + state->offsets.soilMoistureOffsetPermille);
        if (val < 0) {
            val = 0;
        } else if (val > 1000) {
            val = 1000;
        }
        *soilMoisturePermille = (int16_t)val;
    }

    if (illuminanceRaw != NULL) {
        int64_t scaled = ((int64_t)(*illuminanceRaw) * state->offsets.illuminanceScalePercent) / 100;
        if (scaled > INT32_MAX) {
            scaled = INT32_MAX;
        }
        *illuminanceRaw = (int32_t)scaled;
    }

    if (tankLiquid != NULL) {
        if (state->offsets.forceTankEmpty) {
            *tankLiquid = false;
        }
    }

    if (soilHealth != NULL) {
        if (state->offsets.forceSoilSensorError) {
            *soilHealth = SENSOR_HEALTH_OUT_OF_RANGE;
        }
    }
}
