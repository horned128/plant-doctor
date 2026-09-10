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

bool LcdUi_Init(void);                                      /* LcdUi_InitのAPI */
bool LcdUi_Recover(void);                                   /* LcdUi_RecoverのAPI */
bool LcdUi_IsReady(void);                                   /* LcdUi_IsReadyのAPI */
bool LcdUi_ShowBoardTest(void);                             /* LcdUi_ShowBoardTestのAPI */
bool LcdUi_ShowSwitch(uint8_t pressedMask);                 /* LcdUi_ShowSwitchのAPI */
bool LcdUi_ShowPumpStatus(const char *message);             /* ポンプ状態表示API */
bool LcdUi_ShowSensorPage(const PLANT_SENSOR_SNAPSHOT *snapshot, uint8_t page); /* センサー表示API */
bool LcdUi_ShowError(PLANT_DOCTOR_ERROR error);             /* LcdUi_ShowErrorのAPI */

#endif /* LCD_UI_H */
