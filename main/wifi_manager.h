#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_wifi.h"

// Function prototypes
void wifi_manager_init(void);
void wifi_manager_start_ap(void);
void wifi_manager_connect_sta(const char* ssid, const char* password);
void wifi_manager_stop(void);
bool wifi_manager_is_connected(void);

#endif // WIFI_MANAGER_H
