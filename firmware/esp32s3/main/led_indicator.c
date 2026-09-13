#include "led_indicator.h"
#include "app_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "led_indicator";
static led_indicator_state_t s_current_state = LED_STATE_INIT;

/* Simple bit-bang or RMT RGB LED driver for WS2812B */
static void ws2812_send_pixel(uint8_t r, uint8_t g, uint8_t b) {
    /* Send 24 bits (GRB order) */
    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
    
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);
    
    for (int i = 23; i >= 0; i--) {
        if (grb & (1 << i)) {
            /* T1H: ~0.8us High, T1L: ~0.45us Low */
            gpio_set_level(ATOM_S3_RGB_LED_GPIO, 1);
            esp_rom_delay_us(1);
            gpio_set_level(ATOM_S3_RGB_LED_GPIO, 0);
        } else {
            /* T0H: ~0.4us High, T0L: ~0.85us Low */
            gpio_set_level(ATOM_S3_RGB_LED_GPIO, 1);
            /* Short pulse */
            gpio_set_level(ATOM_S3_RGB_LED_GPIO, 0);
            esp_rom_delay_us(1);
        }
    }
    portEXIT_CRITICAL(&mux);
    esp_rom_delay_us(60); /* Reset latch > 50us */
}

static void led_task(void *pvParameters) {
    while (1) {
        switch (s_current_state) {
            case LED_STATE_INIT:
                ws2812_send_pixel(20, 20, 20); /* White dim */
                vTaskDelay(pdMS_TO_TICKS(500));
                ws2812_send_pixel(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            case LED_STATE_CONNECTING:
                ws2812_send_pixel(30, 20, 0); /* Amber blink */
                vTaskDelay(pdMS_TO_TICKS(300));
                ws2812_send_pixel(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(300));
                break;
            case LED_STATE_OK:
                ws2812_send_pixel(0, 30, 0); /* Solid green */
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            case LED_STATE_ACTIVE_TX:
                ws2812_send_pixel(0, 30, 30); /* Cyan flash */
                vTaskDelay(pdMS_TO_TICKS(100));
                ws2812_send_pixel(0, 30, 0); /* Return to green */
                s_current_state = LED_STATE_OK;
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            case LED_STATE_WATERING:
                ws2812_send_pixel(0, 0, 40); /* Blue solid */
                vTaskDelay(pdMS_TO_TICKS(200));
                break;
            case LED_STATE_ERROR:
                ws2812_send_pixel(40, 0, 0); /* Red blink */
                vTaskDelay(pdMS_TO_TICKS(200));
                ws2812_send_pixel(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(200));
                break;
        }
    }
}

void led_indicator_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ATOM_S3_RGB_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(ATOM_S3_RGB_LED_GPIO, 0);

    xTaskCreate(led_task, "led_task", 2048, NULL, 1, NULL);
    ESP_LOGI(TAG, "LED indicator initialized on GPIO %d", ATOM_S3_RGB_LED_GPIO);
}

void led_indicator_set_state(led_indicator_state_t state) {
    s_current_state = state;
}

void led_indicator_set_color(uint8_t red, uint8_t green, uint8_t blue) {
    ws2812_send_pixel(red, green, blue);
}
