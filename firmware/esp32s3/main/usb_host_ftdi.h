#ifndef USB_HOST_FTDI_H
#define USB_HOST_FTDI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize USB Host stack and FTDI FT2232H driver.
 * @return true on success
 */
bool usb_host_ftdi_init(void);

/**
 * @brief Check if DT-EBML63Q2557 (FT2232H) is connected.
 */
bool usb_host_ftdi_is_connected(void);

/**
 * @brief Send raw string/command to DT-EBML63Q2557.
 */
int usb_host_ftdi_write(const char *data, size_t len);

/**
 * @brief Read line or chunk from DT-EBML63Q2557 with timeout.
 */
int usb_host_ftdi_read_line(char *buffer, size_t max_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* USB_HOST_FTDI_H */
