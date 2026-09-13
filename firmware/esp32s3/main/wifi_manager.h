#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void wifi_manager_init(void);
bool wifi_manager_is_connected(void);
const char* wifi_manager_get_ip_string(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MANAGER_H */
