/** =================================================================*
 * @file   Board.h
 * @brief  基板制御API
 * ================================================================= */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>                                        /* 標準Cの真偽値型 */
#include <stdint.h>                                         /* 標準Cの固定幅整数型 */
#include "PlantDoctorStatus.h"                              /* PlantDoctorStatusのAPIと型定義 */

PLANT_DOCTOR_ERROR Board_Init(void);                        /* Board_InitのAPI */
bool Board_Process10Ms(void);                               /* Board_Process10MsのAPI */
bool Board_Take10MsTick(void);                              /* Board_Take10MsTickのAPI */
bool Board_TakeTickOverflow(void);                          /* Board_TakeTickOverflowのAPI */
uint8_t Board_GetPressedSwitchMask(void);                   /* Board_GetPressedSwitchMaskのAPI */
bool Board_IsPowerHeld(void);                               /* Board_IsPowerHeldのAPI */
void Board_ServiceWatchdog(void);                           /* Board_ServiceWatchdogのAPI */

#endif /* BOARD_H */
