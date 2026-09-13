#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LED_STATE_INIT = 0,     /* White pulsing */
    LED_STATE_CONNECTING,   /* Yellow pulsing */
    LED_STATE_OK,           /* Green solid */
    LED_STATE_ACTIVE_TX,    /* Cyan quick flash */
    LED_STATE_WATERING,     /* Blue solid */
    LED_STATE_ERROR         /* Red blink */
} led_indicator_state_t;

void led_indicator_init(void);
void led_indicator_set_state(led_indicator_state_t state);
void led_indicator_set_color(uint8_t red, uint8_t green, uint8_t blue);

#endif /* LED_INDICATOR_H */
