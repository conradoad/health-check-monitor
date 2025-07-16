#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "config.h"
#include "health_checker.h"
#include "wifi_manager.h"
#include "gpio_control.h"

static const char *TAG = "HEALTH_CHECKER";

// Global variables
static TimerHandle_t health_check_timer = NULL;
static char health_check_url[MAX_URL_LENGTH];
static uint32_t check_interval_ms;
static bool is_running = false;
static bool last_health_status = false;

// Function prototypes
static void health_check_timer_callback(TimerHandle_t xTimer);
static void health_check_task(void *pvParameters);
static esp_err_t http_event_handler(esp_http_client_event_t *evt);

void health_checker_start(const char* url, uint32_t interval_ms)
{
    ESP_LOGI(TAG, "Starting health checker");
    ESP_LOGI(TAG, "URL: %s", url);
    ESP_LOGI(TAG, "Interval: %d ms", interval_ms);
    
    if (is_running) {
        health_checker_stop();
    }
    
    // Save parameters
    strncpy(health_check_url, url, sizeof(health_check_url) - 1);
    health_check_url[sizeof(health_check_url) - 1] = '\0';
    check_interval_ms = interval_ms;
    
    // Create timer for periodic health checks
    health_check_timer = xTimerCreate(
        "health_check_timer",
        pdMS_TO_TICKS(check_interval_ms),
        pdTRUE,  // Auto-reload
        NULL,
        health_check_timer_callback
    );
    
    if (health_check_timer != NULL) {
        xTimerStart(health_check_timer, 0);
        is_running = true;
        ESP_LOGI(TAG, "Health checker started successfully");
        
        // Don't perform initial health check immediately
        // Let the timer callback handle it when WiFi is connected
        ESP_LOGI(TAG, "Waiting for WiFi connection to start health checks");
    } else {
        ESP_LOGE(TAG, "Failed to create health check timer");
    }
}

void health_checker_stop(void)
{
    if (is_running) {
        ESP_LOGI(TAG, "Stopping health checker");
        
        if (health_check_timer != NULL) {
            xTimerStop(health_check_timer, 0);
            xTimerDelete(health_check_timer, 0);
            health_check_timer = NULL;
        }
        
        is_running = false;
        last_health_status = false;
        
        // Turn off relay when stopping
        gpio_control_set_relay(false);
        
        ESP_LOGI(TAG, "Health checker stopped");
    }
}

bool health_checker_is_running(void)
{
    return is_running;
}

bool health_checker_get_last_status(void)
{
    return last_health_status;
}

void health_checker_on_wifi_connected(void)
{
    // Perform immediate health check when WiFi connection is established
    if (is_running && wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "WiFi connected, performing immediate health check");
        xTaskCreate(health_check_task, "health_check_task", 4096, NULL, 5, NULL);
    }
}

static void health_check_timer_callback(TimerHandle_t xTimer)
{
    // Only perform health check if WiFi is connected
    if (wifi_manager_is_connected()) {
        ESP_LOGD(TAG, "WiFi connected, performing health check");
        xTaskCreate(health_check_task, "health_check_task", 4096, NULL, 5, NULL);
    } else {
        ESP_LOGD(TAG, "WiFi not connected, skipping health check");
        
        // Set relay to OFF when WiFi is not connected
        last_health_status = false;
        gpio_control_set_relay(false);
    }
}

static void health_check_task(void *pvParameters)
{
    // Double check WiFi connection before proceeding
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi disconnected during health check task creation, aborting");
        last_health_status = false;
        gpio_control_set_relay(false);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Performing health check: %s", health_check_url);
    
    // Check if URL is HTTPS - convert to HTTP for ESP8266 compatibility
    char modified_url[MAX_URL_LENGTH];
    if (strncmp(health_check_url, "https://", 8) == 0) {
        // Convert HTTPS to HTTP for ESP8266 compatibility
        snprintf(modified_url, sizeof(modified_url), "http://%s", health_check_url + 8);
        ESP_LOGW(TAG, "Converting HTTPS to HTTP for ESP8266 compatibility: %s", modified_url);
    } else {
        strncpy(modified_url, health_check_url, sizeof(modified_url) - 1);
        modified_url[sizeof(modified_url) - 1] = '\0';
    }
    
    esp_http_client_config_t config = {
        .url = modified_url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,  // 10 seconds timeout
        .method = HTTP_METHOD_GET,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        last_health_status = false;
        gpio_control_set_relay(false);
        vTaskDelete(NULL);
        return;
    }
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status: %d", status_code);
        
        if (status_code == 200) {
            ESP_LOGI(TAG, "Health check successful");
            last_health_status = true;
            gpio_control_set_relay(true);
        } else {
            ESP_LOGW(TAG, "Health check failed with status: %d", status_code);
            last_health_status = false;
            gpio_control_set_relay(false);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        last_health_status = false;
        gpio_control_set_relay(false);
    }
    
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}
