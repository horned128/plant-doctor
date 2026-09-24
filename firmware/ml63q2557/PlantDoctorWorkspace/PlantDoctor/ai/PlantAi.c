/** =================================================================*
 * @file   PlantAi.c
 * @brief  植物状態AI (特徴量・妥当性・診断・ストレス算出の統合)
 * ================================================================= */
#include "PlantAi.h"                                        /* PlantAiのAPIと型定義 */
#include "PlantFeature.h"                                   /* PlantFeatureのAPIと型定義 */
#include "PlantDoctorConfig.h"                              /* アプリケーション設定 */
#include "SensorManager.h"                                  /* センサー管理API */
#include "SoilCalibration.h"                                /* 土壌水分校正変換API */
#include "SensorPlausibility.h"                             /* センサー妥当性判定API */
#include "PlantDiagnosis.h"                                 /* 植物診断API */
#include "PlantStress.h"                                    /* ストレス算出API */
#include "WateringResponse.h"                              /* 給水応答自己診断API */
#include "PumpControl.h"                                   /* ポンプ制御API */
#include "Board.h"                                         /* ウォッチドッグ制御API */
#if defined(__arm__)
#include "solistAi.h"                                      /* ROHM Solist-AI アクセラレータAPI */
#include "smpl_common.h"                                   /* 共通周辺機能クロック制御API (AI_PERI) */
#define SOLIST_AI_FEATURE_DIM              (8U)
#define SOLIST_AI_BUSY_TIMEOUT_LOOPS       (50000UL)
static bool s_solistAiInitialized = false;
static bfloat16 s_solistAiInput[SOLIST_AI_FEATURE_DIM];
static ODL_Parameters s_solistAiParams;
static uint8_t s_solistAiDivergenceCount = 0U;
#endif
#include <stddef.h>                                         /* NULL定義 */
#include <string.h>                                         /* memcpy定義 */

static PLANT_FEATURE_STATE s_featureState;                  /**< 特徴量抽出内部状態 */
static PLANT_FEATURE_VECTOR s_featureVector;                /**< 最新特徴量ベクトル */
static SENSOR_PLAUSIBILITY_STATE s_plausibilityState;       /**< 妥当性判定状態 */
static PLANT_DIAGNOSIS_STATE s_diagnosisState;              /**< 診断推定状態 */
static PLANT_DIAGNOSIS_CONFIG s_diagConfig;                 /**< 診断設定 */
static PLANT_STRESS_CONFIG s_stressConfig;                  /**< ストレス設定 */
static WATERING_RESPONSE_STATE s_wateringRespState;         /**< 給水応答状態 */
static WATERING_RESPONSE_CONFIG s_wateringRespConfig;       /**< 給水応答設定 */
static uint32_t s_aiTick;                                   /**< 10ms Tickカウンタ */
static uint8_t s_stressScore;                               /**< 最新ストレススコア */
static uint32_t s_solistAiTrainCount = 0U;                  /**< 累積オンデバイス学習回数 */
static uint16_t s_solistAiLatestLossPpm = 250U;             /**< 最新再構成損失 (PPM: 0..10000) */
static uint8_t  s_solistAiPhase = 0U;                       /**< 学習フェーズ (0:Profiling, 1:Stabilizing, 2:Monitoring) */

/**
 * @brief センサー生値を Q8 形式の 0〜256 (実数 0.0〜1.0) に正規化
 */
static inline int16_t NormalizeToQ8(int32_t val, int32_t minVal, int32_t maxVal) {
    if (val <= minVal) {
        return 0;
    }
    if (val >= maxVal) {
        return 256;
    }
    return (int16_t)(((val - minVal) * 256L) / (maxVal - minVal));
}

/** =================================================================*
 * @brief  PlantAi_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PlantAi_Init(void) {
    PlantFeature_Reset(&s_featureState);
    PlantFeature_GetVector(&s_featureState, &s_featureVector);

    SensorPlausibility_Reset(&s_plausibilityState);
    PlantDiagnosis_Reset(&s_diagnosisState);

    /* 診断しきい値の初期設定（マクロ値から注入） */
    s_diagConfig.dryThresholdPermille = PLANT_DOCTOR_DIAG_DRY_THRESHOLD_PERMILLE;
    s_diagConfig.heatStressTempDelta = PLANT_DOCTOR_DIAG_HEAT_DELTA_CENTIC;
    s_diagConfig.heatStressRatePerHour = PLANT_DOCTOR_DIAG_HEAT_RATE_PER_HOUR;
    s_diagConfig.lowLightAccumulatedThreshold = PLANT_DOCTOR_DIAG_LOW_LIGHT_ACCUM;
    s_diagConfig.rootUptakeSoilMinPermille = PLANT_DOCTOR_DIAG_ROOT_UPTAKE_SOIL_PERMILLE;
    s_diagConfig.rootUptakeTempDelta = PLANT_DOCTOR_DIAG_ROOT_UPTAKE_DELTA_CENTIC;
    s_diagConfig.soilDryRatePerHour = PLANT_DOCTOR_DIAG_SOIL_DRY_RATE_PER_HOUR;
    s_diagConfig.soilWetRatePerHour = PLANT_DOCTOR_DIAG_SOIL_WET_RATE_PER_HOUR;

    /* ストレス計算しきい値・重みの初期設定 */
    s_stressConfig.baseStressScore = PLANT_DOCTOR_STRESS_BASE_SCORE;
    s_stressConfig.weightSoil = 100U;
    s_stressConfig.weightHeat = 100U;
    s_stressConfig.weightLight = 80U;
    s_stressConfig.weightHumidity = 60U;
    s_stressConfig.maxDominanceWeight = PLANT_DOCTOR_STRESS_MAX_DOMINANCE_WEIGHT;
    s_stressConfig.baselineSoilMoisturePermille = PLANT_DOCTOR_STRESS_BASELINE_SOIL_PERMILLE;
    s_stressConfig.baselineTempDeltaCentiC = PLANT_DOCTOR_STRESS_BASELINE_TEMP_DELTA;
    s_stressConfig.maxHeatDeltaRangeCentiC = PLANT_DOCTOR_STRESS_MAX_HEAT_DELTA_RANGE;
    s_stressConfig.baselineIlluminanceAccum = PLANT_DOCTOR_STRESS_BASELINE_LIGHT_ACCUM;
    s_stressConfig.baselineHumidityCentiPercent = PLANT_DOCTOR_STRESS_BASELINE_HUMIDITY;

    /* 給水応答設定の初期設定 */
    WateringResponse_Reset(&s_wateringRespState);
    s_wateringRespConfig.immediateSeconds = PLANT_DOCTOR_RESP_IMMEDIATE_SECONDS;
    s_wateringRespConfig.delayedSeconds = PLANT_DOCTOR_RESP_DELAYED_SECONDS;
    s_wateringRespConfig.tempDeltaSeconds = PLANT_DOCTOR_RESP_TEMP_DELTA_SECONDS;
    s_wateringRespConfig.timeoutSeconds = PLANT_DOCTOR_RESP_TIMEOUT_SECONDS;
    s_wateringRespConfig.immediateTicks = (uint32_t)PLANT_DOCTOR_RESP_IMMEDIATE_SECONDS * 100U;
    s_wateringRespConfig.delayedTicks = (uint32_t)PLANT_DOCTOR_RESP_DELAYED_SECONDS * 100U;
    s_wateringRespConfig.tempDeltaTicks = (uint32_t)PLANT_DOCTOR_RESP_TEMP_DELTA_SECONDS * 100U;
    s_wateringRespConfig.timeoutTicks = (uint32_t)PLANT_DOCTOR_RESP_TIMEOUT_SECONDS * 100U;
    s_wateringRespConfig.minSoilRecoveryPermille = PLANT_DOCTOR_RESP_MIN_SOIL_RECOVERY_PERMILLE;
    s_wateringRespConfig.minTempDropCentic = PLANT_DOCTOR_RESP_MIN_TEMP_DROP_CENTIC;
    s_wateringRespConfig.degradeRequiredCycles = PLANT_DOCTOR_DEGRADE_REQUIRED_CYCLES;
    s_wateringRespConfig.degradeMinGainPermille = PLANT_DOCTOR_DEGRADE_MIN_GAIN_PERMILLE;
    s_aiTick = 0U;

    s_stressScore = PLANT_STRESS_UNKNOWN;

#if defined(__arm__)
    smpl_enablePeripheral(AI_PERI);
    for (volatile uint32_t d = 0U; d < 1000U; d++) {
        __NOP();
    }
    s_solistAiParams.inputSize = SOLIST_AI_FEATURE_DIM;
    s_solistAiParams.hiddenSize = 64U;
    s_solistAiParams.outputSize = SOLIST_AI_FEATURE_DIM;
    s_solistAiParams.forgettingFactor = 0x3F80; /* 1.0 (bfloat16) - prevents RLS covariance wind-up */
    s_solistAiParams.activationFunction = ODL_ACTV_SIGMOID;
    s_solistAiParams.lossFunction = ODL_LOSS_MSE;
    s_solistAiParams.seed = 1U;
    s_solistAiParams.scaleAlpha = 0x3F80;       /* 1.0 (bfloat16) */
    s_solistAiParams.scaleGamma = 0;
    s_solistAiParams.leakRate = 0;
    OSUAD_Initialize(&s_solistAiParams, 1U);
    s_solistAiInitialized = true;
    s_solistAiDivergenceCount = 0U;
#endif

    return true;
}

/** =================================================================*
 * @brief  PlantAi_Process10Ms処理
 * ================================================================= */
void PlantAi_Process10Ms(void) {
    PLANT_SENSOR_SNAPSHOT snapshot;
    ++s_aiTick;

    if (SensorManager_TakeNewSample(&snapshot)) {
        PLANT_FEATURE_INPUT featureInput;
        SENSOR_PLAUSIBILITY_INPUT plausInput;
        SENSOR_HEALTH_REPORT healthReport;
        PLANT_DIAGNOSIS_INPUT diagInput;
        PLANT_STRESS_OUTPUT stressOutput;
        WATERING_RESPONSE_INPUT respInput;
        WATERING_RESPONSE respResult;

        /* 1. 特徴量抽出の更新 */
        featureInput.soilMoisturePermille = (int32_t)SoilCalibration_ToPermille(
            snapshot.soilMoistureRaw,
            PLANT_DOCTOR_SOIL_CAL_DEFAULT_DRY,
            PLANT_DOCTOR_SOIL_CAL_DEFAULT_WET);
        featureInput.soilMoistureValid = snapshot.soilMoistureValid;
        featureInput.leafTemperatureCentiC = snapshot.leafTemperatureCentiC;
        featureInput.leafTemperatureValid = snapshot.leafTemperatureValid;
        featureInput.airTemperatureCentiC = snapshot.airTemperatureCentiC;
        featureInput.airTemperatureValid = snapshot.airTemperatureValid;
        featureInput.relativeHumidityCentiPercent = snapshot.relativeHumidityCentiPercent;
        featureInput.relativeHumidityValid = snapshot.airTemperatureValid;
        featureInput.illuminanceRaw = snapshot.illuminanceRaw;
        featureInput.illuminanceValid = snapshot.illuminanceValid;
        featureInput.wateringOccurred = false;

        PlantFeature_Update(&s_featureState, &featureInput);
        PlantFeature_GetVector(&s_featureState, &s_featureVector);

        /* 2. センサー妥当性・整合性の評価 */
        plausInput.soilMoistureRaw = snapshot.soilMoistureRaw;
        plausInput.soilMoistureValid = snapshot.soilMoistureValid;
        plausInput.leafTemperatureCentiC = snapshot.leafTemperatureCentiC;
        plausInput.leafTemperatureValid = snapshot.leafTemperatureValid;
        plausInput.airTemperatureCentiC = snapshot.airTemperatureCentiC;
        plausInput.airTemperatureValid = snapshot.airTemperatureValid;
        plausInput.relativeHumidityCentiPercent = snapshot.relativeHumidityCentiPercent;
        plausInput.relativeHumidityValid = snapshot.airTemperatureValid;
        plausInput.illuminanceRaw = snapshot.illuminanceRaw;
        plausInput.illuminanceValid = snapshot.illuminanceValid;
        plausInput.soilDryCalibration = PLANT_DOCTOR_SOIL_CAL_DEFAULT_DRY;
        plausInput.soilWetCalibration = PLANT_DOCTOR_SOIL_CAL_DEFAULT_WET;

        SensorPlausibility_Evaluate(&s_plausibilityState, &plausInput);
        SensorPlausibility_GetReport(&s_plausibilityState, &healthReport);

        /* 3. 給水応答・自己診断の更新 (P6-1, P6-2) */
        respInput.soilMoisturePermille = (int16_t)featureInput.soilMoisturePermille;
        respInput.leafAirDeltaCentic = (int16_t)s_featureVector.leafAirTemperatureDelta;
        respInput.soilMoistureValid = featureInput.soilMoistureValid;
        respInput.leafAirDeltaValid = featureInput.leafTemperatureValid && featureInput.airTemperatureValid;
        respInput.nowSeconds = snapshot.timestampSeconds;
        respInput.currentTick = s_aiTick;

        WateringResponse_Update(&s_wateringRespState, &respInput, &s_wateringRespConfig);
        respResult = WateringResponse_GetResult(&s_wateringRespState);

        /* 4. 原因候補・土壌傾向の推定 */
        diagInput.features = s_featureVector;
        diagInput.soilHealth = healthReport.soilHealth;
        diagInput.leafHealth = healthReport.leafHealth;
        diagInput.airHumHealth = healthReport.airHumHealth;
        diagInput.luxHealth = healthReport.luxHealth;
        diagInput.isWateringActive = PumpControl_IsOn();
        diagInput.isWateringFailed = (respResult == WATERING_RESPONSE_FAILED);
        diagInput.isSoilDegraded = WateringResponse_IsSoilDegraded(&s_wateringRespState);
        diagInput.wateringDidNotCoolLeaf = WateringResponse_DidNotCoolLeaf(&s_wateringRespState);

        PlantDiagnosis_Update(&s_diagnosisState, &diagInput, &s_diagConfig);

        /* 5. ストレスバロメータの算出 */
        PlantStress_Evaluate(&s_featureVector, &healthReport, &s_stressConfig, &stressOutput);
        s_stressScore = stressOutput.stressScore;

        /* 6. Solist-AI オンデバイス推論および定常学習 */
#if defined(__arm__)
        if (s_solistAiInitialized) {
            int16_t rawFeatures[SOLIST_AI_FEATURE_DIM];
            rawFeatures[0] = NormalizeToQ8(featureInput.soilMoisturePermille, 0, 1000);
            rawFeatures[1] = NormalizeToQ8(snapshot.leafTemperatureCentiC, 1000, 4000);
            rawFeatures[2] = NormalizeToQ8(snapshot.airTemperatureCentiC, 1000, 4000);
            rawFeatures[3] = NormalizeToQ8(s_featureVector.leafAirTemperatureDelta, -400, 200);
            rawFeatures[4] = NormalizeToQ8(snapshot.relativeHumidityCentiPercent, 2000, 10000);
            rawFeatures[5] = NormalizeToQ8(snapshot.illuminanceRaw, 0, 2000);
            rawFeatures[6] = NormalizeToQ8(s_featureVector.leafTemperatureRatePerHour, -500, 500);
            rawFeatures[7] = NormalizeToQ8(s_featureVector.soilMoistureRatePerHour, -200, 200);

            ODL_ToBfloat16(s_solistAiInput, rawFeatures, 8U, SOLIST_AI_FEATURE_DIM);

            OSUAD_StartPredict(0U, s_solistAiInput);
            uint32_t waitLoops = SOLIST_AI_BUSY_TIMEOUT_LOOPS;
            while (OSUAD_IsBusy() && (waitLoops > 0UL)) {
                Board_ServiceWatchdog();
                --waitLoops;
            }
            if (waitLoops > 0UL) {
                bfloat16 loss = OSUAD_GetLoss();
                /* bfloat16 (16-bit) -> float -> PPM (0..10000) */
                float fLoss = 0.0f;
                uint32_t rawLoss32 = ((uint32_t)(uint16_t)loss) << 16;
                memcpy(&fLoss, &rawLoss32, sizeof(float));
                /* Guard against NaN / Inf (exponent bits all 1) */
                if (((rawLoss32 & 0x7F800000UL) == 0x7F800000UL) || (fLoss > 1.0f)) {
                    fLoss = 1.0f;
                } else if (fLoss < 0.0f) {
                    fLoss = 0.0f;
                }
                s_solistAiLatestLossPpm = (uint16_t)(fLoss * 10000.0f);

                /* 平常時（健康状態かつ低ストレスかつセンサ正常）にオンデバイス学習 */
                if (s_diagnosisState.status == PLANT_STATUS_HEALTHY &&
                    healthReport.soilHealth == SENSOR_HEALTH_OK &&
                    healthReport.leafHealth == SENSOR_HEALTH_OK &&
                    healthReport.airHumHealth == SENSOR_HEALTH_OK &&
                    healthReport.luxHealth == SENSOR_HEALTH_OK &&
                    s_stressScore < 30U) {
                    /* 数値発散ウォッチドッグ: 健全時に損失1.0が10周期連続した場合は自律リセット */
                    if (fLoss >= 1.0f) {
                        if (++s_solistAiDivergenceCount >= 10U) {
                            OSUAD_Initialize(&s_solistAiParams, 1U);
                            s_solistAiDivergenceCount = 0U;
                            s_solistAiLatestLossPpm = 250U;
                        }
                    } else {
                        s_solistAiDivergenceCount = 0U;
                    }
                    OSUAD_StartTrain(0U, s_solistAiInput);
                    waitLoops = SOLIST_AI_BUSY_TIMEOUT_LOOPS;
                    while (OSUAD_IsBusy() && (waitLoops > 0UL)) {
                        Board_ServiceWatchdog();
                        --waitLoops;
                    }
                    if (waitLoops > 0UL) {
                        ++s_solistAiTrainCount;
                        if (s_solistAiTrainCount < 100U) {
                            s_solistAiPhase = 0U; /* PROFILING */
                        } else if (s_solistAiTrainCount < 500U) {
                            s_solistAiPhase = 1U; /* STABILIZING */
                        } else {
                            s_solistAiPhase = 2U; /* MONITORING */
                        }
                    } else {
                        s_solistAiInitialized = false;
                    }
                }
            } else {
                /* 加速器タイムアウト時は以降のAI実行を安全に抑止しシステム動作を継続 */
                s_solistAiInitialized = false;
            }
        }
#endif
    }
}

/** =================================================================*
 * @brief  PlantAi_IsAnomaly処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PlantAi_IsAnomaly(void) {
    return (s_diagnosisState.status != PLANT_STATUS_HEALTHY);
}

/** =================================================================*
 * @brief  最新特徴量ベクトルの取得
 * @param[out] vector 特徴量ベクトル出力先
 * ================================================================= */
void PlantAi_GetFeatureVector(PLANT_FEATURE_VECTOR *vector) {
    if (vector != NULL) {
        *vector = s_featureVector;
    }
}

/** =================================================================*
 * @brief  最新診断ステータスの取得
 * @return 診断ステータス
 * ================================================================= */
PLANT_STATUS PlantAi_GetStatus(void) {
    return PlantDiagnosis_GetStatus(&s_diagnosisState);
}

/** =================================================================*
 * @brief  最新土壌傾向の取得
 * @return 土壌変化傾向
 * ================================================================= */
SOIL_TREND PlantAi_GetSoilTrend(void) {
    return PlantDiagnosis_GetSoilTrend(&s_diagnosisState);
}

/** =================================================================*
 * @brief  異常センサーグループの取得
 * @return 異常センサーグループ
 * ================================================================= */
DIAGNOSIS_FAILED_SENSOR PlantAi_GetFailedSensor(void) {
    return PlantDiagnosis_GetFailedSensor(&s_diagnosisState);
}

/** =================================================================*
 * @brief  最新ストレススコア(0-100)の取得
 * @return ストレススコア (0-100、または PLANT_STRESS_UNKNOWN)
 * ================================================================= */
uint8_t PlantAi_GetStressScore(void) {
    return s_stressScore;
}

/** =================================================================*
 * @brief  給水イベントをAIへ通知
 * @param[in] event       給水イベント
 * @param[in] currentTick 現在のTick
 * ================================================================= */
void PlantAi_NotifyWatering(const PUMP_WATERING_EVENT *event, uint32_t currentTick) {
    WateringResponse_NotifyWatering(&s_wateringRespState, event, currentTick);
}

/** =================================================================*
 * @brief  最新の給水応答自己診断結果を取得
 * @return 給水応答結果
 * ================================================================= */
WATERING_RESPONSE PlantAi_GetWateringResponse(void) {
    return WateringResponse_GetResult(&s_wateringRespState);
}

/** =================================================================*
 * @brief  給水失敗状態を手動解除
 * ================================================================= */
void PlantAi_ClearWateringFailure(void) {
    WateringResponse_ClearFailure(&s_wateringRespState);
}

/** =================================================================*
 * @brief  累積オンデバイス学習ステップ数の取得
 * ================================================================= */
uint32_t PlantAi_GetSolistTrainCount(void) {
    return s_solistAiTrainCount;
}

/** =================================================================*
 * @brief  最新再構成損失 (PPM) の取得
 * ================================================================= */
uint16_t PlantAi_GetSolistLossPpm(void) {
    return s_solistAiLatestLossPpm;
}

/** =================================================================*
 * @brief  学習フェーズの取得 (0:Profiling, 1:Stabilizing, 2:Monitoring)
 * ================================================================= */
uint8_t PlantAi_GetSolistPhase(void) {
    return s_solistAiPhase;
}
