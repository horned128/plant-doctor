#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void log_capture_init(void);
void log_capture_get(char *dst, size_t dst_size);
void web_server_start(void);
void web_server_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* WEB_SERVER_H */
