#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	LED_CONTROL_1 = 0,
	LED_CONTROL_2,
	LED_CONTROL_3,
	LED_CONTROL_COUNT
} LED_CONTROL_ID;

void LedControl_Init(void);
void LedControl_Set(LED_CONTROL_ID id, bool on);
void LedControl_Toggle(LED_CONTROL_ID id);
void LedControl_AllOff(void);

#endif /* LED_CONTROL_H */
