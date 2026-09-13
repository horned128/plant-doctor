/** =================================================================*
 * @file   ebml_i2c_slave.c
 * @brief  ATOMS3 Lite I2C Slave Driver for DT-EBML Telemetry
 * ================================================================= */
#include "ebml_i2c_slave.h"
#include "ebml_i2c_proto.h"
#include "ebml_client.h"
#include "app_config.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ebml_i2c_slave";

#define I2C_SLAVE_PORT          I2C_NUM_0
#define I2C_SLAVE_SDA_IO        GPIO_NUM_1   /* Grove White / G1 */
#define I2C_SLAVE_SCL_IO        GPIO_NUM_2   /* Grove Yellow / G2 */
#define I2C_SLAVE_RX_BUF_LEN    512
#define I2C_SLAVE_TX_BUF_LEN    512

static volatile uint8_t s_pending_command = 0;
static volatile uint32_t s_last_pkt_tick = 0;
static volatile bool s_i2c_active = false;


bool ebml_i2c_slave_is_active(void) {
    if (!s_i2c_active) return false;
    uint32_t now = (uint32_t)xTaskGetTickCount();
    if ((now - s_last_pkt_tick) > pdMS_TO_TICKS(2500)) {
        s_i2c_active = false;
        return false;
    }
    return true;
}

void ebml_i2c_slave_queue_command(uint8_t cmd) {
    s_pending_command = cmd;
    ebml_i2c_cmd_pkt_t pkt = {
        .command = cmd,
        .reserved = 0
    };
    /* Pre-load I2C slave TX buffer for master read */
    i2c_slave_write_buffer(I2C_SLAVE_PORT, (const uint8_t *)&pkt, sizeof(pkt), 0);
    ESP_LOGI(TAG, "Queued I2C command to DT-EBML: %u", (unsigned int)cmd);
}

static void ebml_i2c_slave_task(void *pvParameters) {
    uint8_t rx_buf[128];
    uint8_t window[64];
    size_t window_len = 0;

    ESP_LOGI(TAG, "I2C Slave task running on port %d, SDA=GPIO%d, SCL=GPIO%d, Addr=0x%02X",
             I2C_SLAVE_PORT, I2C_SLAVE_SDA_IO, I2C_SLAVE_SCL_IO, EBML_I2C_SLAVE_ADDR);

    /* Prime TX buffer with a default empty command */
    ebml_i2c_cmd_pkt_t initial_resp = { .command = 0, .reserved = 0 };
    i2c_slave_write_buffer(I2C_SLAVE_PORT, (const uint8_t *)&initial_resp, sizeof(initial_resp), 0);

    while (1) {
        int len = i2c_slave_read_buffer(I2C_SLAVE_PORT, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(50));
        if (len > 0) {
            /* Append to window buffer */
            if (window_len + (size_t)len <= sizeof(window)) {
                memcpy(&window[window_len], rx_buf, (size_t)len);
                window_len += (size_t)len;
            } else {
                window_len = 0;
                if ((size_t)len <= sizeof(window)) {
                    memcpy(window, rx_buf, (size_t)len);
                    window_len = (size_t)len;
                }
            }

            /* Search for valid packet in window */
            while (window_len >= sizeof(ebml_i2c_telemetry_pkt_t)) {
                uint16_t magic = (uint16_t)window[0] | ((uint16_t)window[1] << 8);
                if (magic != EBML_I2C_PKT_MAGIC) {
                    memmove(&window[0], &window[1], window_len - 1);
                    window_len--;
                    continue;
                }

                uint8_t pkt_len = window[3];
                if (pkt_len != sizeof(ebml_i2c_telemetry_pkt_t)) {
                    memmove(&window[0], &window[1], window_len - 1);
                    window_len--;
                    continue;
                }

                ebml_i2c_telemetry_pkt_t *pkt = (ebml_i2c_telemetry_pkt_t *)window;
                uint16_t expected_csum = ebml_i2c_calc_checksum(pkt, (uint8_t)(sizeof(ebml_i2c_telemetry_pkt_t) - sizeof(uint16_t)));
                if (pkt->checksum != expected_csum) {
                    ESP_LOGW(TAG, "I2C packet checksum mismatch: got 0x%04X, expected 0x%04X", pkt->checksum, expected_csum);
                    memmove(&window[0], &window[1], window_len - 1);
                    window_len--;
                    continue;
                }

                /* Valid telemetry packet received! */
                s_last_pkt_tick = (uint32_t)xTaskGetTickCount();
                s_i2c_active = true;

                ebml_client_update_from_i2c(pkt);

                /* Reload response packet for master read */
                ebml_i2c_cmd_pkt_t resp = {
                    .command = s_pending_command,
                    .reserved = 0
                };
                s_pending_command = 0;
                i2c_slave_write_buffer(I2C_SLAVE_PORT, (const uint8_t *)&resp, sizeof(resp), 0);

                /* Shift consumed packet out of window */
                size_t rem = window_len - sizeof(ebml_i2c_telemetry_pkt_t);
                if (rem > 0) {
                    memmove(&window[0], &window[sizeof(ebml_i2c_telemetry_pkt_t)], rem);
                }
                window_len = rem;
            }
        }

        /* Check timeout */
        if (s_i2c_active) {
            uint32_t now = (uint32_t)xTaskGetTickCount();
            if ((now - s_last_pkt_tick) > pdMS_TO_TICKS(2500)) {
                ESP_LOGW(TAG, "I2C communication timeout (>2.5s without packet)");
                s_i2c_active = false;
                ebml_client_set_i2c_disconnected();
            }
        }
    }
}

esp_err_t ebml_i2c_slave_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = EBML_I2C_SLAVE_ADDR,
        .slave.maximum_speed = 100000,
    };

    esp_err_t err = i2c_param_config(I2C_SLAVE_PORT, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_SLAVE_PORT, conf.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    xTaskCreate(ebml_i2c_slave_task, "ebml_i2c_slave", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "I2C Slave initialized (Addr=0x%02X, SDA=GPIO%d, SCL=GPIO%d)",
             EBML_I2C_SLAVE_ADDR, I2C_SLAVE_SDA_IO, I2C_SLAVE_SCL_IO);
    return ESP_OK;
}
