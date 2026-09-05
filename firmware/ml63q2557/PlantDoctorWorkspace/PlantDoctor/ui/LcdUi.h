#ifndef LCD_UI_H
#define LCD_UI_H

#include <stdbool.h>
#include <stdint.h>

#include "PlantDoctorStatus.h"

bool LcdUi_Init(void);
bool LcdUi_IsReady(void);
bool LcdUi_ShowBoardTest(void);
bool LcdUi_ShowSwitch(uint8_t pressedMask);
bool LcdUi_ShowError(PLANT_DOCTOR_ERROR error);

#endif /* LCD_UI_H */
