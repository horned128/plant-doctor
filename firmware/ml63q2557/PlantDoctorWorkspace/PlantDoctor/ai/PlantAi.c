/** =================================================================*
 * @file   PlantAi.c
 * @brief  植物状態AI
 * ================================================================= */
#include "PlantAi.h"                                        /* PlantAiのAPIと型定義 */
#include "PlantFeature.h"                                   /* PlantFeatureのAPIと型定義 */

static PLANT_FEATURE_VECTOR s_feature;                      /**< モジュール内部状態 */

/** =================================================================*
 * @brief  PlantAi_Init処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PlantAi_Init(void) {
    PlantFeature_Reset(&s_feature);
    return true;
}

/** =================================================================*
 * @brief  PlantAi_Process10Ms処理
 * ================================================================= */
void PlantAi_Process10Ms(void) {
    /* MlTask、前処理、異常検出器は今後接続する。 */
}

/** =================================================================*
 * @brief  PlantAi_IsAnomaly処理
 * @return 実行結果または取得値
 * ================================================================= */
bool PlantAi_IsAnomaly(void) {
    return false;
}
