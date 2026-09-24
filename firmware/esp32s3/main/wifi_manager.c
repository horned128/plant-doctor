/** =================================================================*
 * @file   wifi_manager.c
 * @brief  Wi-Fi Connection & mDNS Manager
 * ================================================================= */
#include "wifi_manager.h"
#include "app_config.h"
#include "led_indicator.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include <string.h>

#include "web_server.h"

static const char *TAG = "wifi_manager";
static bool s_connected = false;
static char s_ip_str[32] = {0};
static int s_retry_num = 0;

static void start_services_task(void *pvParameters) {
    (void)pvParameters;
    /* Start mDNS service */
    mdns_init();
    mdns_hostname_set(CONFIG_MDNS_HOST_NAME);
    mdns_instance_name_set("Plant Doctor Gateway");
    mdns_service_add("PlantDoctor", "_http", "_tcp", 80, NULL, 0);
    ESP_LOGI(TAG, "mDNS responder started: http://%s.local", CONFIG_MDNS_HOST_NAME);

    /* Start HTTP & WebSocket server after IP is acquired */
    web_server_start();

    vTaskDelete(NULL);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        led_indicator_set_state(LED_STATE_CONNECTING);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        web_server_stop();
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying Wi-Fi connect (%d/%d)...", s_retry_num, CONFIG_ESP_MAXIMUM_RETRY);
        } else {
            ESP_LOGW(TAG, "Wi-Fi connection failed, retrying in 5s...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            s_retry_num = 0;
            esp_wifi_connect();
        }
        led_indicator_set_state(LED_STATE_ERROR);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Connected! IP Address: %s", s_ip_str);
        s_connected = true;
        s_retry_num = 0;
        led_indicator_set_state(LED_STATE_OK);

        /* Offload heavy mDNS and Web server initialization from sys_evt to avoid stack overflow */
        xTaskCreate(start_services_task, "start_srv", 4096, NULL, 5, NULL);
    }
}

void wifi_manager_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Wi-Fi Station initialized for SSID: %s", CONFIG_ESP_WIFI_SSID);
}

bool wifi_manager_is_connected(void) {
    return s_connected;
}

const char* wifi_manager_get_ip_string(void) {
    return s_ip_str;
}
