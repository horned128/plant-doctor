/** =================================================================*
 * @file   web_server.c
 * @brief  HTTP & WebSocket Server for Plant Doctor Gateway
 * ================================================================= */
#include "web_server.h"
#include "ebml_client.h"
#include "app_config.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "index_html.h"

static const char *TAG = "web_server";
static httpd_handle_t s_server = NULL;

/* In-memory circular log buffer */
#define LOG_RING_SIZE 8192
static char s_log_ring[LOG_RING_SIZE];
static size_t s_log_write_idx = 0;
static size_t s_log_total_written = 0;
static portMUX_TYPE s_log_spinlock = portMUX_INITIALIZER_UNLOCKED;
static vprintf_like_t s_default_vprintf = NULL;

static int custom_vprintf(const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int ret = 0;
    if (s_default_vprintf) {
        ret = s_default_vprintf(fmt, copy);
    }
    va_end(copy);

    char temp[256];
    int len = vsnprintf(temp, sizeof(temp), fmt, args);
    if (len > 0) {
        if (len >= (int)sizeof(temp)) {
            len = sizeof(temp) - 1;
        }
        portENTER_CRITICAL(&s_log_spinlock);
        for (int i = 0; i < len; i++) {
            s_log_ring[s_log_write_idx] = temp[i];
            s_log_write_idx = (s_log_write_idx + 1) % LOG_RING_SIZE;
        }
        s_log_total_written += (size_t)len;
        portEXIT_CRITICAL(&s_log_spinlock);
    }
    return ret;
}

void log_capture_init(void) {
    s_default_vprintf = esp_log_set_vprintf(custom_vprintf);
}

void log_capture_get(char *dst, size_t dst_size) {
    if (!dst || dst_size == 0) return;
    dst[0] = '\0';
    portENTER_CRITICAL(&s_log_spinlock);
    size_t count = (s_log_total_written < LOG_RING_SIZE) ? s_log_total_written : LOG_RING_SIZE;
    if (count >= dst_size) {
        count = dst_size - 1;
    }
    size_t start = (s_log_write_idx + LOG_RING_SIZE - count) % LOG_RING_SIZE;
    for (size_t i = 0; i < count; i++) {
        dst[i] = s_log_ring[(start + i) % LOG_RING_SIZE];
    }
    dst[count] = '\0';
    portEXIT_CRITICAL(&s_log_spinlock);
}


static int format_telemetry_json(const plant_telemetry_t *t, char *buf, size_t max_len) {
    return snprintf(buf, max_len,
        "{\"type\":\"telemetry\","
        "\"timestamp\":%lu,"
        "\"seq\":%lu,"
        "\"stress\":%u,"
        "\"status\":\"%s\","
        "\"soil_trend\":\"%s\","
        "\"air_temp\":%.2f,"
        "\"humidity\":%.2f,"
        "\"leaf_temp\":%.2f,"
        "\"leaf_air_diff\":%.2f,"
        "\"soil_raw\":%u,"
        "\"lux\":%u,"
        "\"tank_liquid\":%s,"
        "\"pump_on\":%s,"
        "\"demo_mode\":%s,"
        "\"valid\":%s,"
        "\"ebml_connected\":%s,"
        "\"conn_type\":\"%s\","
        "\"ai_train_count\":%u,"
        "\"ai_loss\":%.4f,"
        "\"ai_phase\":%u,"
        "\"ai_anomaly_score\":%u,"
        "\"leaf_temp_rate\":%.2f,"
        "\"soil_rate\":%d}",
        (unsigned long)t->timestamp,
        (unsigned long)t->sampleSequence,
        (unsigned int)t->stressScore,
        t->status,
        t->soilTrend,
        (double)t->airTemperatureCentiC / 100.0,
        (double)t->relativeHumidityCentiPercent / 100.0,
        (double)t->leafTemperatureCentiC / 100.0,
        (double)t->leafAirDiffCentiC / 100.0,
        (unsigned int)t->soilMoistureRaw,
        (unsigned int)t->illuminanceRaw,
        t->tankLiquidDetected ? "true" : "false",
        t->pumpOn ? "true" : "false",
        t->demoMode ? "true" : "false",
        t->valid ? "true" : "false",
        t->ebml_connected ? "true" : "false",
        t->conn_type,
        (unsigned int)t->aiTrainCount,
        (double)t->aiLoss,
        (unsigned int)t->aiPhase,
        (unsigned int)t->aiAnomalyScore,
        (double)t->leafTempRatePerHour,
        (int)t->soilMoistureRatePerHour);
}


/* Handler for GET / (Serve embedded Dashboard HTML) */
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    httpd_resp_send(req, (const char *)INDEX_HTML, INDEX_HTML_LEN);
    return ESP_OK;
}

/* Handler for GET /api/telemetry (JSON) */
static esp_err_t telemetry_get_handler(httpd_req_t *req) {
    plant_telemetry_t t;
    ebml_client_get_latest_telemetry(&t);

    char json_buf[768];
    format_telemetry_json(&t, json_buf, sizeof(json_buf));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, json_buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Handler for POST /api/water */
static esp_err_t water_post_handler(httpd_req_t *req) {
    bool ok = ebml_client_trigger_watering();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    if (ok) {
        httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"watering_triggered\"}", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"failed_or_rejected\"}", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

/* Handler for POST /api/demo?enable=1/0 */
static esp_err_t demo_post_handler(httpd_req_t *req) {
    char buf[32];
    bool enable = true;
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        if (strstr(buf, "enable=0") != NULL) {
            enable = false;
        }
    }
    bool ok = ebml_client_set_demo_mode(enable);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, ok ? "{\"status\":\"ok\"}" : "{\"status\":\"error\"}", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* WebSocket handler for /ws */
static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket client handshake done");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t rx_buf[128];
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = rx_buf;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, sizeof(rx_buf) - 1);
    if (ret != ESP_OK) {
        return ret;
    }

    rx_buf[ws_pkt.len] = '\0';
    ESP_LOGI(TAG, "WS Received: %s", (char*)rx_buf);

    if (strcmp((char*)rx_buf, "water") == 0) {
        ebml_client_trigger_watering();
    } else if (strcmp((char*)rx_buf, "demo_on") == 0) {
        ebml_client_set_demo_mode(true);
    } else if (strcmp((char*)rx_buf, "demo_off") == 0) {
        ebml_client_set_demo_mode(false);
    }

    return ESP_OK;
}

/* Background task to push telemetry to all connected WebSocket clients */
static void ws_broadcast_task(void *pvParameters) {
    char json_buf[768];
    plant_telemetry_t t;

    while (1) {
        if (s_server && ebml_client_get_latest_telemetry(&t)) {
            format_telemetry_json(&t, json_buf, sizeof(json_buf));

            /* Get all client FDs and send frame */
            size_t max_clients = 8;
            int client_fds[8];
            if (httpd_get_client_list(s_server, &max_clients, client_fds) == ESP_OK) {
                for (size_t i = 0; i < max_clients; ++i) {
                    if (httpd_ws_get_fd_info(s_server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
                        httpd_ws_frame_t ws_pkt = {
                            .final = true,
                            .fragmented = false,
                            .type = HTTPD_WS_TYPE_TEXT,
                            .payload = (uint8_t *)json_buf,
                            .len = strlen(json_buf)
                        };
                        httpd_ws_send_frame_async(s_server, client_fds[i], &ws_pkt);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static TaskHandle_t s_ws_task_handle = NULL;

static esp_err_t logs_get_handler(httpd_req_t *req) {
    char *buf = malloc(LOG_RING_SIZE + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    log_capture_get(buf, LOG_RING_SIZE + 1);
    httpd_resp_set_type(req, "text/plain; charset=utf-8");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    free(buf);
    return ESP_OK;
}

void web_server_start(void) {
    if (s_server != NULL) {
        ESP_LOGW(TAG, "Server already running");
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_open_sockets = 7;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting HTTP/WS server on port %d...", config.server_port);
    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s (0x%x)", esp_err_to_name(ret), ret);
        return;
    }

    httpd_uri_t uri_root = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = root_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_root);

    httpd_uri_t uri_telemetry = {
        .uri      = "/api/telemetry",
        .method   = HTTP_GET,
        .handler  = telemetry_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_telemetry);

    httpd_uri_t uri_logs = {
        .uri      = "/api/logs",
        .method   = HTTP_GET,
        .handler  = logs_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_logs);

    httpd_uri_t uri_water = {
        .uri      = "/api/water",
        .method   = HTTP_POST,
        .handler  = water_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_water);

    httpd_uri_t uri_demo = {
        .uri      = "/api/demo",
        .method   = HTTP_POST,
        .handler  = demo_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(s_server, &uri_demo);

    httpd_uri_t uri_ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true
    };
    httpd_register_uri_handler(s_server, &uri_ws);


    if (s_ws_task_handle == NULL) {
        xTaskCreate(ws_broadcast_task, "ws_broadcast", 4096, NULL, 2, &s_ws_task_handle);
    }
    ESP_LOGI(TAG, "HTTP & WebSocket server started successfully on port %d", config.server_port);
}

void web_server_stop(void) {
    if (s_server != NULL) {
        ESP_LOGI(TAG, "Stopping HTTP server...");
        httpd_stop(s_server);
        s_server = NULL;
    }
}
