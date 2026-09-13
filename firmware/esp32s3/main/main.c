/** =================================================================*
 * @file   main.c
 * @brief  Plant Doctor ATOMS3 Lite Gateway Main Entrypoint
 * ================================================================= */
#include <stdio.h>
#include <string.h>
#include "app_config.h"
#include "led_indicator.h"
#include "usb_host_ftdi.h"
#include "ebml_client.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

/* Button monitor task (GPIO 41 on ATOMS3 Lite) */
static void button_task(void *pvParameters) {
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << ATOM_S3_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_conf);

    int last_level = 1;
    while (1) {
        int level = gpio_get_level(ATOM_S3_BUTTON_GPIO);
        if (last_level == 1 && level == 0) {
            /* Button pressed: Trigger manual watering test */
            ESP_LOGI(TAG, "ATOMS3 Lite front button pressed! Triggering watering...");
            ebml_client_trigger_watering();
            vTaskDelay(pdMS_TO_TICKS(500)); /* Debounce */
        }
        last_level = level;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void) {
    /* 0. Initialize Log Capture for remote debugging */
    log_capture_init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, " Plant Doctor Gateway (ATOMS3 Lite)");
    ESP_LOGI(TAG, " Connecting DT-EBML63Q2557 to LAN/PC");
    ESP_LOGI(TAG, "========================================");


    /* 1. Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. Initialize RGB LED Indicator */
    led_indicator_init();
    led_indicator_set_state(LED_STATE_INIT);

    /* 3. Initialize Wi-Fi Connection */
    wifi_manager_init();

    /* 4. USB Host is disabled in favor of dedicated I2C slave telemetry via Grove */
    // usb_host_ftdi_init();

    /* 5. Initialize EBML Client & Start Polling */
    ebml_client_init();
    ebml_client_start_task();

    /* 6. Start Front Button Monitor Task */
    xTaskCreate(button_task, "button_task", 2048, NULL, 1, NULL);

    ESP_LOGI(TAG, "Gateway system running. Ready for monitoring.");
}
