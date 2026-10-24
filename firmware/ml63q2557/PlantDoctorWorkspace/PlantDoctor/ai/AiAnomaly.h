/** =================================================================*
 * @file   AiAnomaly.h
 * @brief  Solist-AI の再構成誤差による「いつもと違う」判定
 * @author Plant Doctor team
 * @date   2026-10
 *
 * 健康なときの再構成誤差から、その株の誤差の水準（平均）とばらつきを学ぶ。
 * 現在の誤差がそこからどれだけ外れているかを異常度 0〜100 にし、
 * 一定サンプル続いたら「いつもと違う」と判定する（ヒステリシス付き）。
 * ハードウェアに依存しない整数演算のみで、ホストテストできる。
 * ================================================================= */
#ifndef AI_ANOMALY_H
#define AI_ANOMALY_H

#include <stdbool.h>
#include <stdint.h>

#define AI_ANOMALY_LOSS_INVALID_PPM   (10000U)  /* この値以上の誤差は発散・無効として扱う */

typedef struct {
    uint8_t  baselineShift;         /* 平均・ばらつきの指数移動平均の係数 1/2^shift */
    uint32_t minBaselineSamples;    /* 判定を始めるまでに必要な健康サンプル数 */
    uint16_t minDeviationPpm;       /* ばらつきの下限 [ppm] */
    uint8_t  deviationScale;        /* 異常度100に相当する、ばらつきの倍数 */
    uint8_t  onScore;               /* 「いつもと違う」の開始スコア */
    uint8_t  offScore;              /* 解除スコア */
    uint16_t onSamples;             /* 開始に必要な連続サンプル数 */
    uint16_t offSamples;            /* 解除に必要な連続サンプル数 */
    uint16_t relearnEverySamples;   /* いつもと違う間は、このサンプル数に1回だけ学ぶ */
    uint16_t relearnAfterSamples;   /* いつもと違うがこのサンプル数続いたら、新しいいつもとして毎回学ぶ */
} AI_ANOMALY_CONFIG;

typedef struct {
    int32_t  meanQ16;               /* 健康時の誤差の平均 [ppm、16ビット固定小数点] */
    int32_t  deviationQ16;          /* 健康時の誤差のばらつき（平均絶対偏差）[ppm、16ビット固定小数点] */
    uint32_t baselineSamples;       /* 平均に使った健康サンプル数 */
    uint8_t  score;                 /* 異常度 0〜100（準備中は0） */
    bool     isAnomaly;             /* 「いつもと違う」判定 */
    uint16_t aboveCount;            /* 開始スコア以上の連続数 */
    uint16_t belowCount;            /* 解除スコア未満の連続数 */
    uint16_t deviatingSamples;      /* ルール上は健康のまま、いつもから外れている連続数 */
} AI_ANOMALY_STATE;

void AiAnomaly_Reset(AI_ANOMALY_STATE *state);
void AiAnomaly_Update(AI_ANOMALY_STATE *state, const AI_ANOMALY_CONFIG *config,
                      uint16_t lossPpm, bool isHealthySample);
bool AiAnomaly_IsReady(const AI_ANOMALY_STATE *state, const AI_ANOMALY_CONFIG *config);
uint8_t AiAnomaly_GetScore(const AI_ANOMALY_STATE *state);
bool AiAnomaly_IsAnomaly(const AI_ANOMALY_STATE *state);
bool AiAnomaly_AllowTraining(AI_ANOMALY_STATE *state, const AI_ANOMALY_CONFIG *config, bool isRuleSafe);
void AiAnomaly_ClearJudgement(AI_ANOMALY_STATE *state);

#endif /* AI_ANOMALY_H */
