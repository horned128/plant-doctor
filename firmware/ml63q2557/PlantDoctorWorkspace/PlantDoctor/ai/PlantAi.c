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
#include "AiAnomaly.h"                                     /* 再構成誤差による「いつもと違う」判定 */
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
static AI_ANOMALY_STATE s_aiAnomaly;                        /**< その株の「いつも」からのずれの判定状態 */
static AI_ANOMALY_CONFIG s_aiAnomalyConfig;                 /**< 「いつもと違う」判定の設定 */
static uint16_t s_aiPostWateringHoldSamples;                /**< 給水後、AIの判定と学習を休む残りサンプル数 */
static int16_t s_demoLeafOffsetCentiC;                      /**< デモ: 診断用の葉温オフセット [1/100 ℃] */
static int16_t s_demoSoilOffsetPermille;                    /**< デモ: 診断用の土壌水分オフセット [‰] */
static bool s_demoForceSoilError;                           /**< デモ: 診断用に土壌センサー異常を模す */

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
 * @brief  デモモードの診断用オフセット適用
 * @param[in,out] features 診断・ストレス用の特徴量（コピー）
 * @param[in,out] health   診断・ストレス用のセンサー健全性（コピー）
 * @return オフセットを当てたらtrue（その間はAIに学習させない）
 * ================================================================= */
static bool PlantAi_ApplyDemoAdjustment(PLANT_FEATURE_VECTOR *features, SENSOR_HEALTH_REPORT *health) {
    int32_t soil;

    if ((s_demoLeafOffsetCentiC == 0) && (s_demoSoilOffsetPermille == 0) && !s_demoForceSoilError) {
        return false;
    }
    soil = features->soilMoisturePermille + s_demoSoilOffsetPermille;
    features->soilMoisturePermille = (soil < 0) ? 0 : ((soil > 1000) ? 1000 : soil);
    soil = features->soilMoistureMovingAverage + s_demoSoilOffsetPermille;
    features->soilMoistureMovingAverage = (soil < 0) ? 0 : ((soil > 1000) ? 1000 : soil);
    features->leafAirTemperatureDelta += s_demoLeafOffsetCentiC;
    if (s_demoForceSoilError) {
        health->soilHealth = SENSOR_HEALTH_OUT_OF_RANGE;
    }
    return true;
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
    /* 重みは設定ファイルの値を使う（以前はここに 100/100/80/60 を直接書いており、設定が反映されていなかった） */
    s_stressConfig.weightSoil = PLANT_DOCTOR_STRESS_WEIGHT_SOIL;
    s_stressConfig.weightHeat = PLANT_DOCTOR_STRESS_WEIGHT_HEAT;
    s_stressConfig.weightLight = PLANT_DOCTOR_STRESS_WEIGHT_LIGHT;
    s_stressConfig.weightHumidity = PLANT_DOCTOR_STRESS_WEIGHT_HUMIDITY;
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
    s_wateringRespConfig.failedHoldTicks = PLANT_DOCTOR_RESP_FAILED_HOLD_SECONDS * 100UL;
    s_aiTick = 0U;

    s_stressScore = PLANT_STRESS_UNKNOWN;

    /* 「いつもと違う」判定の設定（再構成誤差の、その株の平常値からのずれ） */
    AiAnomaly_Reset(&s_aiAnomaly);
    s_aiPostWateringHoldSamples = 0U;
    s_demoLeafOffsetCentiC = 0;
    s_demoSoilOffsetPermille = 0;
    s_demoForceSoilError = false;
    s_aiAnomalyConfig.baselineShift = PLANT_DOCTOR_AI_ANOMALY_BASELINE_SHIFT;
    s_aiAnomalyConfig.minBaselineSamples = PLANT_DOCTOR_AI_ANOMALY_MIN_BASELINE_SAMPLES;
    s_aiAnomalyConfig.minDeviationPpm = PLANT_DOCTOR_AI_ANOMALY_MIN_DEVIATION_PPM;
    s_aiAnomalyConfig.deviationScale = PLANT_DOCTOR_AI_ANOMALY_DEVIATION_SCALE;
    s_aiAnomalyConfig.onScore = PLANT_DOCTOR_AI_ANOMALY_ON_SCORE;
    s_aiAnomalyConfig.offScore = PLANT_DOCTOR_AI_ANOMALY_OFF_SCORE;
    s_aiAnomalyConfig.onSamples = PLANT_DOCTOR_AI_ANOMALY_ON_SAMPLES;
    s_aiAnomalyConfig.offSamples = PLANT_DOCTOR_AI_ANOMALY_OFF_SAMPLES;
    s_aiAnomalyConfig.relearnEverySamples = PLANT_DOCTOR_AI_ANOMALY_RELEARN_EVERY_SAMPLES;
    s_aiAnomalyConfig.relearnAfterSamples = PLANT_DOCTOR_AI_ANOMALY_RELEARN_AFTER_SAMPLES;

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
        PLANT_FEATURE_VECTOR diagFeatures;
        SENSOR_HEALTH_REPORT diagHealth;
        bool isDemoAdjusted;
        uint16_t calDry;
        uint16_t calWet;

        /* 1. 特徴量抽出の更新 */
        /* 校正値はコンソールの C= で変えられるので、定数ではなく現在の設定を使う */
        SoilCalibration_Get(&calDry, &calWet);
        featureInput.soilMoisturePermille = (int32_t)SoilCalibration_ToPermille(snapshot.soilMoistureRaw, calDry, calWet);
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
        plausInput.soilDryCalibration = calDry;
        plausInput.soilWetCalibration = calWet;

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

        /* 4. 原因候補・土壌傾向の推定
         * デモモードのずらし値は診断とストレスの入力にだけ入れ、AIの入力と給水応答の判定には入れない */
        diagFeatures = s_featureVector;
        diagHealth = healthReport;
        isDemoAdjusted = PlantAi_ApplyDemoAdjustment(&diagFeatures, &diagHealth);
        diagInput.features = diagFeatures;
        diagInput.soilHealth = diagHealth.soilHealth;
        diagInput.leafHealth = diagHealth.leafHealth;
        diagInput.airHumHealth = diagHealth.airHumHealth;
        diagInput.luxHealth = diagHealth.luxHealth;
        diagInput.isWateringActive = PumpControl_IsOn();
        diagInput.isWateringFailed = (respResult == WATERING_RESPONSE_FAILED);
        diagInput.isSoilDegraded = WateringResponse_IsSoilDegraded(&s_wateringRespState);
        diagInput.wateringDidNotCoolLeaf = WateringResponse_DidNotCoolLeaf(&s_wateringRespState);

        PlantDiagnosis_Update(&s_diagnosisState, &diagInput, &s_diagConfig);

        /* 5. ストレスバロメータの算出 */
        PlantStress_Evaluate(&diagFeatures, &diagHealth, &s_stressConfig, &stressOutput);
        s_stressScore = stressOutput.stressScore;

        /* 6. Solist-AI オンデバイス推論および定常学習 */
#if defined(__arm__)
        /* 給水後しばらくは、土壌水分の変化速度に給水の影響が残る（直近60分で計算するため）。
         * その間は「いつもと違う」の判定も学習もしない（給水の影響を異常とも、いつもとも扱わない） */
        bool isPostWateringHold = (s_aiPostWateringHoldSamples > 0U);
        if (isPostWateringHold) {
            --s_aiPostWateringHoldSamples;
        }
        if (s_solistAiInitialized) {
            int16_t rawFeatures[SOLIST_AI_FEATURE_DIM];
            /* 起動直後は変化速度の履歴が短く値が大きく振れるので、履歴がたまるまでは中間値（変化なし）を入れる */
            bool isRateReady = (s_featureState.minuteHistoryCount >= PLANT_DOCTOR_AI_RATE_READY_MINUTES);
            rawFeatures[0] = NormalizeToQ8(featureInput.soilMoisturePermille, 0, 1000);
            rawFeatures[1] = NormalizeToQ8(snapshot.leafTemperatureCentiC, 1000, 4000);
            rawFeatures[2] = NormalizeToQ8(snapshot.airTemperatureCentiC, 1000, 4000);
            rawFeatures[3] = NormalizeToQ8(s_featureVector.leafAirTemperatureDelta, -400, 200);
            rawFeatures[4] = NormalizeToQ8(snapshot.relativeHumidityCentiPercent, 2000, 10000);
            rawFeatures[5] = NormalizeToQ8(snapshot.illuminanceRaw, 0, 6000);
            /* 変化速度は範囲を狭め、その株らしい葉温の上がり方・土の乾き方からのずれを誤差に表れやすくする */
            rawFeatures[6] = isRateReady ? NormalizeToQ8(s_featureVector.leafTemperatureRatePerHour,
                                                         -PLANT_DOCTOR_AI_LEAF_RATE_RANGE_CENTIC,
                                                         PLANT_DOCTOR_AI_LEAF_RATE_RANGE_CENTIC) : 128;
            rawFeatures[7] = isRateReady ? NormalizeToQ8(s_featureVector.soilMoistureRatePerHour,
                                                         -PLANT_DOCTOR_AI_SOIL_RATE_RANGE_PERMILLE,
                                                         PLANT_DOCTOR_AI_SOIL_RATE_RANGE_PERMILLE) : 128;

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
                /* 初期プロファイリング(100回未満)は起動直後の積算照度立ち上がりや日常微乾燥を考慮して
                 * 総合閾値を50に緩和。ただし致命的な土壌乾燥や熱ストレス（部分スコア>=40）が
                 * 発生している場合は絶対に学習させない多層防御インターロックを適用。 */
                bool isRuleSafe = false;
                bool isSafeToTrain;
                /* 給水後（変化速度に給水の影響が残る約1時間半と、給水の効果を測っている間）と、
                 * デモモードで診断の入力をずらしている間は学ばない */
                if (s_diagnosisState.status == PLANT_STATUS_HEALTHY &&
                    !isPostWateringHold &&
                    !isDemoAdjusted &&
                    respResult != WATERING_RESPONSE_MEASURING &&
                    healthReport.soilHealth == SENSOR_HEALTH_OK &&
                    healthReport.leafHealth == SENSOR_HEALTH_OK &&
                    healthReport.airHumHealth == SENSOR_HEALTH_OK &&
                    healthReport.luxHealth == SENSOR_HEALTH_OK) {
                    if (s_stressScore < 30U) {
                        isRuleSafe = true;
                    } else if ((s_solistAiTrainCount < 100U) &&
                               (s_stressScore < 50U) &&
                               (stressOutput.soilPartialScore < 40U) &&
                               (stressOutput.heatPartialScore < 40U)) {
                        isRuleSafe = true;
                    }
                }

                /* その株の「いつも」からのずれ（異常度）を更新する。基準（平常時の誤差の水準とばらつき）は、
                 * ルール上健康で、変化速度の履歴がそろい、モデルの学習が進んだサンプルだけで作る。
                 * 給水後は判定を休み、基準だけを残す */
                if (isPostWateringHold) {
                    AiAnomaly_ClearJudgement(&s_aiAnomaly);
                } else {
                    AiAnomaly_Update(&s_aiAnomaly, &s_aiAnomalyConfig, s_solistAiLatestLossPpm,
                                     isRuleSafe && isRateReady &&
                                     (s_solistAiTrainCount >= PLANT_DOCTOR_AI_ANOMALY_MIN_TRAIN_COUNT));
                }
                isSafeToTrain = isRuleSafe;
                if (PLANT_DOCTOR_AI_ANOMALY_ENABLE) {
                    /* いつもと違う間は学習を絞る（異常をいつもとして覚えない）。
                     * ただし長く続いたら新しいいつもとして学び直す（照明の切り替え・置き場所の変更など） */
                    isSafeToTrain = AiAnomaly_AllowTraining(&s_aiAnomaly, &s_aiAnomalyConfig, isRuleSafe);
                    /* ルール上は健康でも、AIが「いつもと違う」と判定したら WARNING として知らせる */
                    if ((s_diagnosisState.status == PLANT_STATUS_HEALTHY) && AiAnomaly_IsAnomaly(&s_aiAnomaly)) {
                        s_diagnosisState.status = PLANT_STATUS_WARNING;
                    }
                }

                if (isSafeToTrain) {
                    /* 数値発散ウォッチドッグ: 健全時に損失1.0が10周期連続した場合は自律リセット */
                    if (fLoss >= 1.0f) {
                        if (++s_solistAiDivergenceCount >= 10U) {
                            OSUAD_Initialize(&s_solistAiParams, 1U);
                            s_solistAiDivergenceCount = 0U;
                            s_solistAiLatestLossPpm = 250U;
                            /* モデルを作り直したら、学習の段階と異常度の基準も最初から作り直す */
                            s_solistAiTrainCount = 0U;
                            s_solistAiPhase = 0U;
                            AiAnomaly_Reset(&s_aiAnomaly);
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
                        /* AIを止めたら異常度も送らない */
                        AiAnomaly_Reset(&s_aiAnomaly);
                    }
                }
            } else {
                /* 加速器タイムアウト時は以降のAI実行を安全に抑止しシステム動作を継続 */
                s_solistAiInitialized = false;
                /* AIを止めたら異常度も送らない */
                AiAnomaly_Reset(&s_aiAnomaly);
            }
        }
#else
        /* Solist-AI のないホスト環境では、デモの補正有無は学習判定に使わない */
        (void)isDemoAdjusted;
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
    /* 給水応答の経過時間は、判定側（PlantAi_Process10Ms）と同じ s_aiTick で測る */
    (void)currentTick;
    WateringResponse_NotifyWatering(&s_wateringRespState, event, s_aiTick);
    /* 給水後は約1時間半、AIの判定と学習を休む（給水で直前の「いつもと違う」も解除する） */
    s_aiPostWateringHoldSamples = PLANT_DOCTOR_AI_POST_WATERING_HOLD_SAMPLES;
    AiAnomaly_ClearJudgement(&s_aiAnomaly);
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

/** =================================================================*
 * @brief  Solist-AI 異常度取得
 * @return その株の「いつも」からのずれ 0〜100（準備中・給水後の休止中は0）
 * ================================================================= */
uint8_t PlantAi_GetAiAnomalyScore(void) {
    return AiAnomaly_GetScore(&s_aiAnomaly);
}

/** =================================================================*
 * @brief  Solist-AI「いつもと違う」判定取得
 * @return 判定中ならtrue
 * ================================================================= */
bool PlantAi_IsAiAnomaly(void) {
    return AiAnomaly_IsAnomaly(&s_aiAnomaly);
}

/** =================================================================*
 * @brief  デモモードの診断用オフセット設定
 * @details 診断とストレスの入力にだけ加え、Solist-AI の入力・学習・給水応答の判定には使わない。
 * @param[in] leafOffsetCentiC     葉温に加えるオフセット [1/100 ℃]
 * @param[in] soilOffsetPermille   土壌水分に加えるオフセット [‰]
 * @param[in] forceSoilSensorError 土壌センサー異常を模すならtrue
 * ================================================================= */
void PlantAi_SetDemoAdjustment(int16_t leafOffsetCentiC, int16_t soilOffsetPermille, bool forceSoilSensorError) {
    s_demoLeafOffsetCentiC = leafOffsetCentiC;
    s_demoSoilOffsetPermille = soilOffsetPermille;
    s_demoForceSoilError = forceSoilSensorError;
}
