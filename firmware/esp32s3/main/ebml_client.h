#ifndef EBML_CLIENT_H
#define EBML_CLIENT_H

#include "app_config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ebml_client_init(void);

/**
 * @brief Get the latest cached telemetry snapshot (thread-safe).
 */
bool ebml_client_get_latest_telemetry(plant_telemetry_t *telemetry);

/**
 * @brief Trigger watering request on DT-EBML63Q2557.
 */
bool ebml_client_trigger_watering(void);

/**
 * @brief Set demo mode on/off.
 */
bool ebml_client_set_demo_mode(bool enable);

/**
 * @brief Synchronize datetime to DT-EBML63Q2557 RTC.
 */
bool ebml_client_sync_time(uint32_t unix_seconds);

/**
 * @brief Start the periodic polling task.
 */
void ebml_client_start_task(void);

#include "ebml_i2c_proto.h"
void ebml_client_update_from_i2c(const ebml_i2c_telemetry_pkt_t *pkt);
void ebml_client_set_i2c_disconnected(void);

#ifdef __cplusplus
}
#endif

#endif /* EBML_CLIENT_H */
