/** =================================================================*
 * @file   usb_host_ftdi.c
 * @brief  USB Host FTDI Driver for DT-EBML63Q2557 (FT2232H)
 * ================================================================= */
#include "usb_host_ftdi.h"
#include "app_config.h"
#include "led_indicator.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include <string.h>

#include "freertos/semphr.h"
#include "driver/uart.h"
#include "usb/usb_host.h"
#include "usb/cdc_acm_host.h"
#include "usb/vcp_ftdi.h"

static const char *TAG = "usb_host_ftdi";

/* USB Host State */
static cdc_acm_dev_hdl_t s_cdc_hdl = NULL;
static volatile bool s_usb_connected = false;

/* RX Ring Buffer */
#define RX_BUF_SIZE 1024
static uint8_t s_rx_buf[RX_BUF_SIZE];
static size_t s_rx_head = 0;
static size_t s_rx_tail = 0;
static size_t s_rx_count = 0;
static SemaphoreHandle_t s_rx_sem = NULL;
static SemaphoreHandle_t s_rx_mutex = NULL;

/* Note: Grove pins (GPIO 1 & 2) are now dedicated to I2C slave communication with DT-EBML */
static void init_uart_fallback(void) {
    /* No-op: GPIO 1 (SDA) and GPIO 2 (SCL) are used for I2C */
}

/**
 * @brief Store incoming raw byte in circular buffer
 */
static bool cdc_rx_cb(const uint8_t *data, size_t data_len, void *user_arg) {
    (void)user_arg;
    if (!data || data_len == 0 || !s_rx_mutex) return true;

    if (xSemaphoreTake(s_rx_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        for (size_t i = 0; i < data_len; i++) {
            if (s_rx_count < RX_BUF_SIZE) {
                s_rx_buf[s_rx_head] = data[i];
                s_rx_head = (s_rx_head + 1) % RX_BUF_SIZE;
                s_rx_count++;
            }
        }
        xSemaphoreGive(s_rx_mutex);
        if (s_rx_sem) {
            xSemaphoreGive(s_rx_sem);
        }
    }
    return true;
}

/**
 * @brief Retrieve a byte from circular buffer with timeout
 */
static bool rx_buf_get_byte(uint8_t *ch, TickType_t wait_ticks) {
    TickType_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) <= wait_ticks) {
        if (xSemaphoreTake(s_rx_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (s_rx_count > 0) {
                *ch = s_rx_buf[s_rx_tail];
                s_rx_tail = (s_rx_tail + 1) % RX_BUF_SIZE;
                s_rx_count--;
                xSemaphoreGive(s_rx_mutex);
                return true;
            }
            xSemaphoreGive(s_rx_mutex);
        }
        if (s_rx_sem) {
            xSemaphoreTake(s_rx_sem, pdMS_TO_TICKS(10));
        } else {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    return false;
}

/**
 * @brief CDC-ACM device event callback
 */
static void cdc_event_cb(const cdc_acm_host_dev_event_data_t *event, void *user_ctx) {
    (void)user_ctx;
    switch (event->type) {
        case CDC_ACM_HOST_DEVICE_DISCONNECTED:
            ESP_LOGW(TAG, "FTDI device disconnected!");
            s_usb_connected = false;
            if (s_cdc_hdl) {
                cdc_acm_host_close(s_cdc_hdl);
                s_cdc_hdl = NULL;
            }
            led_indicator_set_state(LED_STATE_CONNECTING);
            break;
        case CDC_ACM_HOST_ERROR:
            ESP_LOGE(TAG, "CDC-ACM host error event: %d", event->data.error);
            break;
        default:
            break;
    }
}

/**
 * @brief Background task handling USB Host library events
 */
static void usb_host_lib_task(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "USB Host lib event task started");
    while (1) {
        uint32_t event_flags;
        esp_err_t err = usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (err == ESP_OK) {
            if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
                ESP_LOGD(TAG, "No more USB host clients, freeing devices");
                usb_host_device_free_all();
            }
            if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
                ESP_LOGD(TAG, "All USB devices freed");
            }
        }
    }
}

static void new_usb_dev_cb(usb_device_handle_t usb_dev) {
    const usb_device_desc_t *dev_desc = NULL;
    esp_err_t err = usb_host_get_device_descriptor(usb_dev, &dev_desc);
    if (err == ESP_OK && dev_desc) {
        ESP_LOGI(TAG, "===> USB DEVICE DETECTED! VID: 0x%04X, PID: 0x%04X, Class: 0x%02X",
                 dev_desc->idVendor, dev_desc->idProduct, dev_desc->bDeviceClass);
    } else {
        ESP_LOGW(TAG, "===> USB device detected, but get_desc failed: %s", esp_err_to_name(err));
    }
}

/**
 * @brief Background task managing FT2232H device enumeration & reconnection
 */
static void usb_host_conn_task(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "USB FT2232H connection task started");

    while (1) {
        if (!s_usb_connected) {
            usb_host_lib_info_t lib_info;
            if (usb_host_lib_info(&lib_info) == ESP_OK) {
                ESP_LOGI(TAG, "USB Host state: %d device(s) connected, %d client(s)",
                         lib_info.num_devices, lib_info.num_clients);
            }

            cdc_acm_host_device_config_t dev_cfg = {
                .connection_timeout_ms = 1000,
                .out_buffer_size = 256,
                .in_buffer_size = 0,
                .event_cb = cdc_event_cb,
                .data_cb = cdc_rx_cb,
                .user_arg = NULL,
            };

            cdc_acm_dev_hdl_t dev_hdl = NULL;
            /* FTDI FT2232H: VID=0x0403, PID=0x6010
             * On DT-EBML63Q2557, ML63Q2557 UARTF1 connects to FT2232H Port B (Interface 1) */
            esp_err_t err = ftdi_vcp_open(0x6010, 1, &dev_cfg, &dev_hdl);
            if (err == ESP_OK && dev_hdl) {
                ESP_LOGI(TAG, "FTDI FT2232H connected on Interface 1 (Port B / UARTF1)!");
                s_cdc_hdl = dev_hdl;
                s_usb_connected = true;
                led_indicator_set_state(LED_STATE_OK);
            } else {
                ESP_LOGD(TAG, "ftdi_vcp_open(0x6010, intf 1) returned %s", esp_err_to_name(err));
                /* Also try Interface 0 if Channel A was mapped */
                err = ftdi_vcp_open(0x6010, 0, &dev_cfg, &dev_hdl);
                if (err == ESP_OK && dev_hdl) {
                    ESP_LOGI(TAG, "FTDI FT2232H connected on Interface 0 (Port A)!");
                    s_cdc_hdl = dev_hdl;
                    s_usb_connected = true;
                    led_indicator_set_state(LED_STATE_OK);
                } else {
                    ESP_LOGD(TAG, "ftdi_vcp_open(0x6010, intf 0) returned %s", esp_err_to_name(err));
                    /* Try auto PID */
                    err = ftdi_vcp_open(FTDI_PID_AUTO, 0, &dev_cfg, &dev_hdl);
                    if (err == ESP_OK && dev_hdl) {
                        ESP_LOGI(TAG, "FTDI device connected on auto PID!");
                        s_cdc_hdl = dev_hdl;
                        s_usb_connected = true;
                        led_indicator_set_state(LED_STATE_OK);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

bool usb_host_ftdi_init(void) {
    ESP_LOGI(TAG, "Initializing USB Host / FTDI interface...");

    s_rx_mutex = xSemaphoreCreateMutex();
    s_rx_sem = xSemaphoreCreateBinary();

    /* 1. Install USB Host Library */
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err_t err = usb_host_install(&host_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "usb_host_install failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "USB Host Library installed successfully");
        xTaskCreatePinnedToCore(usb_host_lib_task, "usb_host_lib", 4096, NULL, 20, NULL, 0);

        /* 2. Install CDC-ACM Host Driver */
        const cdc_acm_host_driver_config_t cdc_driver_config = {
            .driver_task_stack_size = 4096,
            .driver_task_priority = 19,
            .xCoreID = 0,
            .new_dev_cb = new_usb_dev_cb,
        };
        err = cdc_acm_host_install(&cdc_driver_config);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "cdc_acm_host_install failed: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "CDC-ACM Host Driver installed successfully");
            /* 3. Start Connection Monitoring Task */
            xTaskCreatePinnedToCore(usb_host_conn_task, "ftdi_conn", 4096, NULL, 5, NULL, 0);
        }
    }

    /* 4. Also initialize Grove UART as secondary fallback */
    init_uart_fallback();

    return true;
}

bool usb_host_ftdi_is_connected(void) {
    return s_usb_connected;
}

int usb_host_ftdi_write(const char *data, size_t len) {
    if (!data || len == 0) return 0;

    if (s_usb_connected && s_cdc_hdl) {
        esp_err_t err = cdc_acm_host_data_tx_blocking(s_cdc_hdl, (const uint8_t *)data, len, 500);
        if (err == ESP_OK) {
            return (int)len;
        }
        ESP_LOGW(TAG, "USB TX error: %s", esp_err_to_name(err));
    }

    return 0;
}

int usb_host_ftdi_read_line(char *buffer, size_t max_len, uint32_t timeout_ms) {
    if (!buffer || max_len == 0) return 0;

    size_t idx = 0;
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t wait_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start_tick) < wait_ticks && idx < (max_len - 1)) {
        uint8_t ch = 0;
        bool have_byte = false;

        if (s_usb_connected) {
            have_byte = rx_buf_get_byte(&ch, pdMS_TO_TICKS(20));
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
            break;
        }

        if (have_byte) {
            if (ch == '\r') {
                continue;
            }
            if (ch == '\n') {
                buffer[idx] = '\0';
                return (int)idx;
            }
            buffer[idx++] = (char)ch;
        }
    }

    buffer[idx] = '\0';
    return (int)idx;
}

