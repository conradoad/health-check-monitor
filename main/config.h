#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define CONFIG_AP_SSID "SONOFF-Monitor"
#define CONFIG_AP_PASSWORD "12345678"
#define CONFIG_AP_CHANNEL 1
#define CONFIG_AP_MAX_CONNECTIONS 4

// GPIO Pin Definitions (SONOFF MINI)
#define GPIO_BUTTON 0
#define GPIO_TX 1
#define GPIO_AVAILABLE 2
#define GPIO_RX 3
#define GPIO_S2 4
#define GPIO_RELAY 12
#define GPIO_BLUE_LED 13
#define GPIO_OTA_JUMPER 16

// Configuration
#define BUTTON_PRESS_TIME_MS 5000  // 5 seconds to enter config mode
#define DEFAULT_HEALTH_CHECK_INTERVAL_MS 30000  // 30 seconds
#define MAX_RETRY_COUNT 3

// HTTP Configuration
#define HTTP_SERVER_PORT 80
#define MAX_URL_LENGTH 256
#define MAX_WIFI_SSID_LENGTH 32
#define MAX_WIFI_PASSWORD_LENGTH 64

// NVS Keys
#define NVS_NAMESPACE "config"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASSWORD "wifi_pass"
#define NVS_KEY_HEALTH_URL "health_url"
#define NVS_KEY_CHECK_INTERVAL "check_interval"
#define NVS_KEY_CONFIGURED "configured"

// Configuration structure
typedef struct {
    char wifi_ssid[MAX_WIFI_SSID_LENGTH];
    char wifi_password[MAX_WIFI_PASSWORD_LENGTH];
    char health_check_url[MAX_URL_LENGTH];
    uint32_t check_interval_ms;
    bool configured;
} device_config_t;

#endif // CONFIG_H
