#ifndef HEALTH_CHECKER_H
#define HEALTH_CHECKER_H

#include <stdbool.h>
#include <stdint.h>

// Function prototypes
void health_checker_start(const char* url, uint32_t interval_ms);
void health_checker_stop(void);
bool health_checker_is_running(void);
bool health_checker_get_last_status(void);
void health_checker_on_wifi_connected(void);  // Notify when WiFi is connected
void health_checker_save_last_status(bool status);  // Save last status to NVS
bool health_checker_load_last_status(void);  // Load last status from NVS

#endif // HEALTH_CHECKER_H
