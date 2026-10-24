/** =================================================================*
 * @file   AiAnomaly.c
 * @brief  Solist-AI の再構成誤差による「いつもと違う」判定の実装
 * @author Plant Doctor team
 * @date   2026-10
 * ================================================================= */
#include "AiAnomaly.h"
#include <stddef.h>

/** =================================================================*
 * @brief  状態を初期化する（モデルを初期化したときも呼ぶ）
 * ================================================================= */
void AiAnomaly_Reset(AI_ANOMALY_STATE *state) {
    if (state == NULL) {
        return;
    }
    state->meanQ16 = 0;
    state->deviationQ16 = 0;
    state->baselineSamples = 0U;
    state->score = 0U;
    state->isAnomaly = false;
    state->aboveCount = 0U;
    state->belowCount = 0U;
    state->deviatingSamples = 0U;
}

/** =================================================================*
 * @brief  判定に必要な健康サンプルがそろったか
 * ================================================================= */
/* 判定（異常度・「いつもと違う」・連続数）だけを消し、健康時の基準（平均・ばらつき・件数）は残す。
 * 給水の直後など、判定を休む間に使う */
void AiAnomaly_ClearJudgement(AI_ANOMALY_STATE *state) {
    if (state == NULL) {
        return;
    }
    state->score = 0U;
    state->isAnomaly = false;
    state->aboveCount = 0U;
    state->belowCount = 0U;
    state->deviatingSamples = 0U;
}

bool AiAnomaly_IsReady(const AI_ANOMALY_STATE *state, const AI_ANOMALY_CONFIG *config) {
    if ((state == NULL) || (config == NULL)) {
        return false;
    }
    return state->baselineSamples >= config->minBaselineSamples;
}

/** =================================================================*
 * @brief  最新の異常度 0〜100
 * ================================================================= */
uint8_t AiAnomaly_GetScore(const AI_ANOMALY_STATE *state) {
    return (state == NULL) ? 0U : state->score;
}

/** =================================================================*
 * @brief  「いつもと違う」判定
 * ================================================================= */
bool AiAnomaly_IsAnomaly(const AI_ANOMALY_STATE *state) {
    return (state != NULL) && state->isAnomaly;
}

/** =================================================================*
 * @brief  このサンプルで Solist-AI を学習させてよいか（AiAnomaly_Update の後に呼ぶ）
 * @param  isRuleSafe  ルール上の学習条件（健康・センサー正常・ストレス低め）を満たすか
 *
 * いつもから外れていなければ毎回学ぶ。外れている間は、異常をいつもとして覚えないよう
 * relearnEverySamples に1回だけ学ぶ。外れたまま relearnAfterSamples 続いたら
 * （照明の切り替えや置き場所の変更など）、新しいいつもとして毎回学び直す。
 * ================================================================= */
bool AiAnomaly_AllowTraining(AI_ANOMALY_STATE *state, const AI_ANOMALY_CONFIG *config, bool isRuleSafe) {
    if ((state == NULL) || (config == NULL) || !isRuleSafe) {
        return false;
    }
    if (!state->isAnomaly && (state->score < config->onScore)) {
        state->deviatingSamples = 0U;
        return true;
    }
    if (state->deviatingSamples < UINT16_MAX) {
        ++state->deviatingSamples;
    }
    if (state->deviatingSamples >= config->relearnAfterSamples) {
        return true;
    }
    return (config->relearnEverySamples > 0U) &&
           ((state->deviatingSamples % config->relearnEverySamples) == 0U);
}

/** =================================================================*
 * @brief  再構成誤差を1件取り込み、異常度と判定を更新する
 * @param  lossPpm          最新の再構成誤差 [ppm]（10000以上は無効）
 * @param  isHealthySample  ルール上も学習インターロック上も健康なサンプルか
 * ================================================================= */
void AiAnomaly_Update(AI_ANOMALY_STATE *state, const AI_ANOMALY_CONFIG *config,
                      uint16_t lossPpm, bool isHealthySample) {
    int32_t xq;
    int32_t diff;
    int32_t deviation;
    int64_t scaled;

    if ((state == NULL) || (config == NULL)) {
        return;
    }
    /* 発散・無効な誤差では何も更新しない（モデルの初期化はウォッチドッグに任せる） */
    if (lossPpm >= AI_ANOMALY_LOSS_INVALID_PPM) {
        return;
    }
    xq = (int32_t)((uint32_t)lossPpm << 16);   /* 9999ppm でも int32 に収まる */

    /* 1. 異常度: 健康時の平均からの上振れを、ばらつきの何倍かで表す */
    if (AiAnomaly_IsReady(state, config)) {
        deviation = state->deviationQ16;
        if (deviation < (int32_t)((uint32_t)config->minDeviationPpm << 16)) {
            deviation = (int32_t)((uint32_t)config->minDeviationPpm << 16);
        }
        diff = xq - state->meanQ16;
        if (diff <= 0) {
            state->score = 0U;
        } else {
            scaled = ((int64_t)diff * 100) / ((int64_t)deviation * (int64_t)config->deviationScale);
            state->score = (uint8_t)((scaled > 100) ? 100 : scaled);
        }
    } else {
        state->score = 0U;
    }

    /* 2. 判定（ヒステリシス）: 開始スコア以上が続いたら異常、解除スコア未満が続いたら解除。
     * 誤差は1秒ごとに揺れるので、条件から1回外れても数え直さず、1つ減らすだけにする。
     * 数は判定に必要な回数で頭打ちにし、長く平常が続いたあとでも開始・解除が遅れないようにする */
    if (state->score >= config->onScore) {
        if (state->aboveCount < config->onSamples) {
            ++state->aboveCount;
        }
        if (state->belowCount > 0U) {
            --state->belowCount;
        }
    } else if (state->score < config->offScore) {
        if (state->belowCount < config->offSamples) {
            ++state->belowCount;
        }
        if (state->aboveCount > 0U) {
            --state->aboveCount;
        }
    } else {
        if (state->aboveCount > 0U) {
            --state->aboveCount;
        }
        if (state->belowCount > 0U) {
            --state->belowCount;
        }
    }
    if (!state->isAnomaly && (state->aboveCount >= config->onSamples)) {
        state->isAnomaly = true;
        state->belowCount = 0U;
    } else if (state->isAnomaly && (state->belowCount >= config->offSamples)) {
        state->isAnomaly = false;
        state->aboveCount = 0U;
    }

    /* 3. 基準の学習: 健康で、いつもから外れていないサンプルだけで平均とばらつきを更新する */
    if (isHealthySample && !state->isAnomaly && (state->score < config->onScore)) {
        if (state->baselineSamples == 0U) {
            state->meanQ16 = xq;
            state->deviationQ16 = 0;
        } else {
            int32_t err = xq - state->meanQ16;
            int32_t absErr = (err < 0) ? -err : err;
            uint32_t n = state->baselineSamples + 1U;
            uint32_t window = 1UL << config->baselineShift;
            if (n < window) {
                /* 立ち上がりは単純平均で早く収束させる */
                state->meanQ16 += err / (int32_t)n;
                state->deviationQ16 += (absErr - state->deviationQ16) / (int32_t)n;
            } else {
                state->meanQ16 += err / (int32_t)window;
                state->deviationQ16 += (absErr - state->deviationQ16) / (int32_t)window;
            }
        }
        if (state->baselineSamples < UINT32_MAX) {
            ++state->baselineSamples;
        }
    }
}
