#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "PlantDoctorStatus.h"

PLANT_DOCTOR_ERROR Board_Init(void);
bool Board_Process10Ms(void);
bool Board_Take10MsTick(void);
bool Board_TakeTickOverflow(void);
uint8_t Board_GetPressedSwitchMask(void);
bool Board_IsPowerHeld(void);
void Board_ServiceWatchdog(void);

#endif /* BOARD_H */
