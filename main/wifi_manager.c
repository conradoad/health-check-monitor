#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "config.h"
#include "wifi_manager.h"
#include "health_checker.h"

static const char *TAG = "WIFI_MANAGER";

// Event group for WiFi events
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static bool s_wifi_initialized = false;
static bool s_wifi_connected = false;
static int s_retry_num = 0;

// Function prototypes
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

void wifi_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi manager");
    
    if (s_wifi_initialized) {
        ESP_LOGI(TAG, "WiFi already initialized");
        return;
    }
    
    // Create event group
    s_wifi_event_group = xEventGroupCreate();
    
    // Initialize TCP/IP adapter
    tcpip_adapter_init();
    
    // Set event handler first
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi manager initialized");
}

void wifi_manager_start_ap(void)
{
    ESP_LOGI(TAG, "Starting WiFi AP mode");
    
    // Stop any existing WiFi connection
    esp_wifi_stop();
    
    // Configure AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_AP_SSID,
            .ssid_len = strlen(CONFIG_AP_SSID),
            .channel = CONFIG_AP_CHANNEL,
            .password = CONFIG_AP_PASSWORD,
            .max_connection = CONFIG_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    
    if (strlen(CONFIG_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP started. SSID: %s, Password: %s", CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
}

void wifi_manager_connect_sta(const char* ssid, const char* password)
{
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
    
    // Stop any existing WiFi connection
    esp_wifi_stop();
    
    // Configure STA
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    s_retry_num = 0;
    s_wifi_connected = false;
    
    ESP_LOGI(TAG, "WiFi connection initiated");
}

void wifi_manager_stop(void)
{
    ESP_LOGI(TAG, "Stopping WiFi");
    esp_wifi_stop();
    s_wifi_connected = false;
}

bool wifi_manager_is_connected(void)
{
    return s_wifi_connected;
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi station started");
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            if (s_retry_num < MAX_RETRY_COUNT) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "Retry to connect to AP (%d/%d)", s_retry_num, MAX_RETRY_COUNT);
            } else {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                ESP_LOGI(TAG, "Failed to connect to AP");
            }
            s_wifi_connected = false;
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
            s_retry_num = 0;
            s_wifi_connected = true;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            
            // Notify health checker that WiFi is connected
            health_checker_on_wifi_connected();
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                     MAC2STR(event->event_info.sta_connected.mac),
                     event->event_info.sta_connected.aid);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                     MAC2STR(event->event_info.sta_disconnected.mac),
                     event->event_info.sta_disconnected.aid);
            break;
        default:
            break;
    }
    return ESP_OK;
}
