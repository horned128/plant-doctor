/** =================================================================*
 * @file   ebml_client.c
 * @brief  DT-EBML63Q2557 Console Client & Protocol Parser
 * ================================================================= */
#include "ebml_client.h"
#include "ebml_i2c_proto.h"
#include "ebml_i2c_slave.h"
#include "usb_host_ftdi.h"
#include "led_indicator.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "ebml_client";

static plant_telemetry_t s_latest_telemetry;
static SemaphoreHandle_t s_telemetry_mutex = NULL;
static SemaphoreHandle_t s_uart_mutex = NULL;

static bool send_cmd_and_get_response(const char *cmd, char *resp_buf, size_t max_len) {
    if (!s_uart_mutex) return false;
    
    if (xSemaphoreTake(s_uart_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "UART mutex timeout");
        return false;
    }

    /* Send command with CRLF */
    char tx_buf[64];
    int len = snprintf(tx_buf, sizeof(tx_buf), "%s\r\n", cmd);
    usb_host_ftdi_write(tx_buf, (size_t)len);

    /* Read line response */
    int read_len = usb_host_ftdi_read_line(resp_buf, max_len, UART_RESPONSE_TIMEOUT_MS);
    xSemaphoreGive(s_uart_mutex);

    return (read_len > 0);
}

static bool parse_sensor_response(const char *resp, plant_telemetry_t *t) {
    /* Expected: OK seq=%u soil=%u leaf=%d air=%d hum=%u lux=%u tank=%u */
    if (strncmp(resp, "OK ", 3) != 0) return false;

    unsigned int seq = 0, soil = 0, hum = 0, lux = 0, tank = 0;
    int leaf = 0, air = 0;

    int matched = sscanf(resp, "OK seq=%u soil=%u leaf=%d air=%d hum=%u lux=%u tank=%u",
                         &seq, &soil, &leaf, &air, &hum, &lux, &tank);
    if (matched >= 7) {
        t->sampleSequence = (uint32_t)seq;
        t->soilMoistureRaw = (uint16_t)soil;
        t->leafTemperatureCentiC = (int16_t)leaf;
        t->airTemperatureCentiC = (int16_t)air;
        t->leafAirDiffCentiC = (int16_t)(leaf - air);
        t->relativeHumidityCentiPercent = (uint16_t)hum;
        t->illuminanceRaw = (uint16_t)lux;
        t->tankLiquidDetected = (tank != 0);
        t->valid = true;
        t->timestamp = (uint32_t)time(NULL);
        return true;
    }
    return false;
}

static bool parse_diagnosis_response(const char *resp, plant_telemetry_t *t) {
    /* Expected: OK stress=%u status=%s soil=%s */
    if (strncmp(resp, "OK ", 3) != 0) return false;

    unsigned int stress = 0;
    char status[32] = {0};
    char soil[16] = {0};

    int matched = sscanf(resp, "OK stress=%u status=%31s soil=%15s",
                         &stress, status, soil);
    if (matched >= 3) {
        t->stressScore = (uint8_t)stress;
        strncpy(t->status, status, sizeof(t->status) - 1);
        strncpy(t->soilTrend, soil, sizeof(t->soilTrend) - 1);
        return true;
    }
    return false;
}

static void ebml_polling_task(void *pvParameters) {
    char resp[128];
    uint32_t loop_count = 0;
    uint32_t fail_count = 0;

    while (1) {
        if (usb_host_ftdi_is_connected()) {
            bool poll_ok = false;

            /* 1. Poll Sensors with 'S' */
            if (send_cmd_and_get_response("S", resp, sizeof(resp))) {
                if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    if (parse_sensor_response(resp, &s_latest_telemetry)) {
                        s_latest_telemetry.ebml_connected = true;
                        strncpy(s_latest_telemetry.conn_type, "USB", sizeof(s_latest_telemetry.conn_type) - 1);
                        poll_ok = true;
                        fail_count = 0;
                        led_indicator_set_state(LED_STATE_ACTIVE_TX);
                    }
                    xSemaphoreGive(s_telemetry_mutex);
                }
            } else {
                ESP_LOGD(TAG, "No response for 'S'");
            }

            if (!poll_ok) {
                fail_count++;
                if (fail_count >= 3) {
                    if (!ebml_i2c_slave_is_active()) {
                        if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                            s_latest_telemetry.ebml_connected = false;
                            xSemaphoreGive(s_telemetry_mutex);
                        }
                    }
                }
            }

            vTaskDelay(pdMS_TO_TICKS(100));

            /* 2. Poll Diagnosis every 2 seconds with 'Q' */
            if ((loop_count % 2) == 0) {
                if (send_cmd_and_get_response("Q", resp, sizeof(resp))) {
                    if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        parse_diagnosis_response(resp, &s_latest_telemetry);
                        xSemaphoreGive(s_telemetry_mutex);
                    }
                }
            }

            /* 3. Poll Pump Status with 'W?' */
            if (send_cmd_and_get_response("W?", resp, sizeof(resp))) {
                unsigned int pump = 0;
                if (sscanf(resp, "OK pump=%u", &pump) == 1) {
                    if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        s_latest_telemetry.pumpOn = (pump != 0);
                        if (s_latest_telemetry.pumpOn) {
                            led_indicator_set_state(LED_STATE_WATERING);
                        }
                        xSemaphoreGive(s_telemetry_mutex);
                    }
                }
            }

            loop_count++;
        } else {
            /* USB not connected: Check if I2C is active */
            if (!ebml_i2c_slave_is_active()) {
                if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    s_latest_telemetry.ebml_connected = false;
                    xSemaphoreGive(s_telemetry_mutex);
                }
                led_indicator_set_state(LED_STATE_CONNECTING);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_INTERVAL_MS));
    }
}

void ebml_client_update_from_i2c(const ebml_i2c_telemetry_pkt_t *pkt) {
    if (!pkt || !s_telemetry_mutex) return;

    if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_latest_telemetry.sampleSequence = pkt->sampleSequence;
        s_latest_telemetry.soilMoistureRaw = pkt->soilMoistureRaw;
        s_latest_telemetry.leafTemperatureCentiC = pkt->leafTemperatureCentiC;
        s_latest_telemetry.airTemperatureCentiC = pkt->airTemperatureCentiC;
        s_latest_telemetry.leafAirDiffCentiC = (int16_t)(pkt->leafTemperatureCentiC - pkt->airTemperatureCentiC);
        s_latest_telemetry.relativeHumidityCentiPercent = pkt->relativeHumidityCentiPercent;
        s_latest_telemetry.illuminanceRaw = pkt->illuminanceRaw;
        s_latest_telemetry.tankLiquidDetected = (pkt->tankLiquidDetected != 0);
        s_latest_telemetry.pumpOn = (pkt->pumpOn != 0);
        s_latest_telemetry.stressScore = pkt->stressScore;

        switch (pkt->statusCode) {
            case 0:  strncpy(s_latest_telemetry.status, "HEALTHY", sizeof(s_latest_telemetry.status)-1); break;
            case 1:  strncpy(s_latest_telemetry.status, "DRY_STRESS", sizeof(s_latest_telemetry.status)-1); break;
            case 2:  strncpy(s_latest_telemetry.status, "HEAT_STRESS", sizeof(s_latest_telemetry.status)-1); break;
            case 3:  strncpy(s_latest_telemetry.status, "LOW_LIGHT", sizeof(s_latest_telemetry.status)-1); break;
            case 4:  strncpy(s_latest_telemetry.status, "ROOT_UPTAKE", sizeof(s_latest_telemetry.status)-1); break;
            case 5:  strncpy(s_latest_telemetry.status, "WATERING", sizeof(s_latest_telemetry.status)-1); break;
            case 6:  strncpy(s_latest_telemetry.status, "WATERING_FAILED", sizeof(s_latest_telemetry.status)-1); break;
            case 7:  strncpy(s_latest_telemetry.status, "SOIL_DEGRADATION", sizeof(s_latest_telemetry.status)-1); break;
            case 8:  strncpy(s_latest_telemetry.status, "WARNING", sizeof(s_latest_telemetry.status)-1); break;
            case 9:  strncpy(s_latest_telemetry.status, "SENSOR_ERROR", sizeof(s_latest_telemetry.status)-1); break;
            case 10: strncpy(s_latest_telemetry.status, "LEARNING", sizeof(s_latest_telemetry.status)-1); break;
            default: snprintf(s_latest_telemetry.status, sizeof(s_latest_telemetry.status), "STATUS_%u", pkt->statusCode); break;
        }

        switch (pkt->soilTrendCode) {
            case 1:  strncpy(s_latest_telemetry.soilTrend, "STABLE", sizeof(s_latest_telemetry.soilTrend)-1); break;
            case 2:  strncpy(s_latest_telemetry.soilTrend, "DRYING", sizeof(s_latest_telemetry.soilTrend)-1); break;
            case 3:  strncpy(s_latest_telemetry.soilTrend, "WETTING", sizeof(s_latest_telemetry.soilTrend)-1); break;
            case 4:  strncpy(s_latest_telemetry.soilTrend, "DEGRADED", sizeof(s_latest_telemetry.soilTrend)-1); break;
            case 0:
            default: strncpy(s_latest_telemetry.soilTrend, "UNKNOWN", sizeof(s_latest_telemetry.soilTrend)-1); break;
        }

        s_latest_telemetry.aiTrainCount = pkt->aiTrainCount;
        s_latest_telemetry.aiLoss = (float)pkt->aiLossPpm / 10000.0f;
        s_latest_telemetry.aiPhase = pkt->aiPhase;
        s_latest_telemetry.aiAnomalyScore = pkt->aiAnomalyScore;
        s_latest_telemetry.leafTempRatePerHour = (float)pkt->leafTempRatePerHour / 100.0f;
        s_latest_telemetry.soilMoistureRatePerHour = pkt->soilMoistureRatePerHour;

        s_latest_telemetry.valid = true;
        s_latest_telemetry.ebml_connected = true;
        strncpy(s_latest_telemetry.conn_type, "I2C", sizeof(s_latest_telemetry.conn_type) - 1);
        s_latest_telemetry.timestamp = (uint32_t)time(NULL);

        if (s_latest_telemetry.pumpOn) {
            led_indicator_set_state(LED_STATE_WATERING);
        } else {
            led_indicator_set_state(LED_STATE_ACTIVE_TX);
        }

        xSemaphoreGive(s_telemetry_mutex);
    }
}

void ebml_client_set_i2c_disconnected(void) {
    if (!usb_host_ftdi_is_connected()) {
        if (s_telemetry_mutex && xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            s_latest_telemetry.ebml_connected = false;
            xSemaphoreGive(s_telemetry_mutex);
        }
        led_indicator_set_state(LED_STATE_CONNECTING);
    }
}

void ebml_client_init(void) {
    s_telemetry_mutex = xSemaphoreCreateMutex();
    s_uart_mutex = xSemaphoreCreateMutex();
    memset(&s_latest_telemetry, 0, sizeof(s_latest_telemetry));
    strcpy(s_latest_telemetry.status, "INIT");
    strcpy(s_latest_telemetry.soilTrend, "UNKNOWN");
    strcpy(s_latest_telemetry.conn_type, "NONE");
    s_latest_telemetry.valid = false;
    s_latest_telemetry.ebml_connected = false;

    /* Initialize I2C Slave communication */
    ebml_i2c_slave_init();
}

bool ebml_client_get_latest_telemetry(plant_telemetry_t *telemetry) {
    if (!telemetry || !s_telemetry_mutex) return false;
    if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    *telemetry = s_latest_telemetry;
    xSemaphoreGive(s_telemetry_mutex);
    return telemetry->valid;
}

bool ebml_client_trigger_watering(void) {
    if (usb_host_ftdi_is_connected()) {
        char resp[64];
        ESP_LOGI(TAG, "Triggering remote watering via UART 'W' command");
        if (send_cmd_and_get_response("W", resp, sizeof(resp))) {
            ESP_LOGI(TAG, "Watering response: %s", resp);
            return (strstr(resp, "OK") != NULL);
        }
    }

    ESP_LOGI(TAG, "Triggering remote watering via I2C command");
    ebml_i2c_slave_queue_command(1);
    return true;
}

bool ebml_client_set_demo_mode(bool enable) {
    char resp[64];
    const char *cmd = enable ? "M=1" : "M=0";
    ESP_LOGI(TAG, "Setting demo mode: %s", cmd);
    if (send_cmd_and_get_response(cmd, resp, sizeof(resp))) {
        if (xSemaphoreTake(s_telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            s_latest_telemetry.demoMode = enable;
            xSemaphoreGive(s_telemetry_mutex);
        }
        return (strstr(resp, "OK") != NULL);
    }
    return false;
}

bool ebml_client_sync_time(uint32_t unix_seconds) {
    struct tm t;
    time_t raw = (time_t)unix_seconds;
    gmtime_r(&raw, &t);

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "T=%04d%02d%02d%02d%02d%02d",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
             t.tm_hour, t.tm_min, t.tm_sec);

    char resp[64];
    ESP_LOGI(TAG, "Syncing RTC datetime: %s", cmd);
    return (send_cmd_and_get_response(cmd, resp, sizeof(resp)) && strstr(resp, "OK") != NULL);
}

void ebml_client_start_task(void) {
    xTaskCreate(ebml_polling_task, "ebml_poll", 4096, NULL, 3, NULL);
    ESP_LOGI(TAG, "EBML polling task started");
}
