/** =================================================================*
 * @file   ebml_i2c_slave.h
 * @brief  ATOMS3 Lite I2C Slave Driver for DT-EBML Telemetry
 * ================================================================= */
#ifndef EBML_I2C_SLAVE_H
#define EBML_I2C_SLAVE_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ebml_i2c_slave_init(void);
void ebml_i2c_slave_queue_command(uint8_t cmd);
bool ebml_i2c_slave_is_active(void);

#ifdef __cplusplus
}
#endif

#endif /* EBML_I2C_SLAVE_H */
