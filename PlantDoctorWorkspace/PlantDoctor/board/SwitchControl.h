#ifndef SWITCH_CONTROL_H
#define SWITCH_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#define SWITCH_CONTROL_PSW1    (1U << 0U)
#define SWITCH_CONTROL_PSW2    (1U << 1U)
#define SWITCH_CONTROL_PSW3    (1U << 2U)
#define SWITCH_CONTROL_PSW4    (1U << 3U)

bool SwitchControl_Init(void);
bool SwitchControl_Process10Ms(void);
uint8_t SwitchControl_GetPressedMask(void);

#endif /* SWITCH_CONTROL_H */
