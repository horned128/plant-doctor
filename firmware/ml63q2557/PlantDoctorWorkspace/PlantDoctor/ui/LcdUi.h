/** =================================================================*
 * @file   LcdUi.h
 * @brief  LCDユーザーインターフェースAPI
 * ================================================================= */
#ifndef LCD_UI_H
#define LCD_UI_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"                              /* PlantDoctorStatusのAPIと型定義 */
#include "SensorManager.h"                                  /* センサー取得APIとスナップショット型 */
#include "PlantDiagnosis.h"                                 /* 診断型 */

typedef struct {
    uint8_t stressScore;                                    /* 0〜100、または PLANT_STRESS_UNKNOWN */
    PLANT_STATUS status;                                    /* 診断ステータス */
    DIAGNOSIS_FAILED_SENSOR failedSensor;                   /* センサー異常時の対象グループ */
    SOIL_TREND soilTrend;                                   /* 土壌水分変化傾向 */
    int32_t leafAirTemperatureDelta;                        /* 葉温－気温差 [1/100 ℃] */
    bool leafAirDeltaValid;                                 /* 葉温－気温差有効フラグ */
    bool isDemoMode;                                        /* デモモード動作中フラグ */
} LCD_DIAGNOSIS_VIEW_DATA;

bool LcdUi_Init(void);                                      /* LcdUi_InitのAPI */
bool LcdUi_Recover(void);                                   /* LcdUi_RecoverのAPI */
bool LcdUi_IsReady(void);                                   /* LcdUi_IsReadyのAPI */
bool LcdUi_ShowBoardTest(void);                             /* LcdUi_ShowBoardTestのAPI */
bool LcdUi_ShowSwitch(uint8_t pressedMask);                 /* LcdUi_ShowSwitchのAPI */
bool LcdUi_ShowPumpStatus(const char *message);             /* ポンプ状態表示API */
bool LcdUi_ShowSensorPage(const PLANT_SENSOR_SNAPSHOT *snapshot, uint8_t page); /* センサー表示API */
bool LcdUi_ShowDiagnosisPage(const LCD_DIAGNOSIS_VIEW_DATA *diagData,
                             const PLANT_SENSOR_SNAPSHOT *snapshot,
                             uint8_t page);                 /* 診断表示API (P4-4) */
bool LcdUi_ShowError(PLANT_DOCTOR_ERROR error);             /* LcdUi_ShowErrorのAPI */

/* 文字列生成純関数 (ホストテスト検証可能、16文字以内+NULL終端) */
void LcdUi_FormatStatus(PLANT_STATUS status, DIAGNOSIS_FAILED_SENSOR failedSensor, char *line, uint8_t lineSize);
void LcdUi_FormatStressLine(uint8_t stressScore, bool isDemoMode, char *line, uint8_t lineSize);
void LcdUi_FormatLeafAirDelta(int32_t deltaCentiC, bool valid, char *line, uint8_t lineSize);
void LcdUi_FormatSoilTrend(SOIL_TREND trend, char *line, uint8_t lineSize);
void LcdUi_FormatPage(const LCD_DIAGNOSIS_VIEW_DATA *diagData,
                      const PLANT_SENSOR_SNAPSHOT *snapshot,
                      uint8_t page,
                      char *line1,
                      char *line2,
                      uint8_t lineSize);
void LcdUi_FormatError(PLANT_DOCTOR_ERROR error, char *line1, char *line2, uint8_t lineSize);

#endif /* LCD_UI_H */

