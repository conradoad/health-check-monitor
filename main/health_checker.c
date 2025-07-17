#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "nvs.h"
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
static void update_health_status(bool status);

void health_checker_start(const char* url, uint32_t interval_ms)
{
    ESP_LOGI(TAG, "Starting health checker");
    ESP_LOGI(TAG, "URL: %s", url);
    ESP_LOGI(TAG, "Interval: %d ms", interval_ms);
    
    if (is_running) {
        health_checker_stop();
    }
    
    // Load last known health status and apply to relay
    last_health_status = health_checker_load_last_status();
    gpio_control_set_relay(last_health_status);
    ESP_LOGI(TAG, "Restored last health status: %s, relay: %s", 
             last_health_status ? "OK" : "FAIL", 
             last_health_status ? "ON" : "OFF");
    
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
        update_health_status(false);  // Turn off relay and save status
        
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
        update_health_status(false);
    }
}

static void health_check_task(void *pvParameters)
{
    // Double check WiFi connection before proceeding
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi disconnected during health check task creation, aborting");
        update_health_status(false);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "Performing health check: %s", health_check_url);
    
    esp_http_client_config_t config = {
        .url = health_check_url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,  // 10 seconds timeout
        .method = HTTP_METHOD_GET,
        .skip_cert_common_name_check = true,  // Skip certificate verification for HTTPS
        .cert_pem = NULL,
        .client_cert_pem = NULL,
        .client_key_pem = NULL,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        update_health_status(false);
        vTaskDelete(NULL);
        return;
    }
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status: %d", status_code);
        
        if (status_code == 200) {
            ESP_LOGI(TAG, "Health check successful");
            update_health_status(true);
        } else {
            ESP_LOGW(TAG, "Health check failed with status: %d", status_code);
            update_health_status(false);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        update_health_status(false);
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

static void update_health_status(bool status)
{
    if (last_health_status != status) {
        last_health_status = status;
        gpio_control_set_relay(status);
        health_checker_save_last_status(status);
        ESP_LOGI(TAG, "Health status updated: %s, relay: %s", 
                 status ? "OK" : "FAIL", 
                 status ? "ON" : "OFF");
    }
}

void health_checker_save_last_status(bool status)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return;
    }
    
    err = nvs_set_u8(nvs_handle, NVS_KEY_LAST_HEALTH_STATUS, status ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving last health status: %s", esp_err_to_name(err));
    } else {
        ESP_LOGD(TAG, "Last health status saved: %s", status ? "OK" : "FAIL");
    }
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

bool health_checker_load_last_status(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error opening NVS handle for reading: %s", esp_err_to_name(err));
        return false;  // Default to false if can't read
    }
    
    uint8_t status_value = 0;
    err = nvs_get_u8(nvs_handle, NVS_KEY_LAST_HEALTH_STATUS, &status_value);
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        bool status = (status_value == 1);
        ESP_LOGI(TAG, "Last health status loaded: %s", status ? "OK" : "FAIL");
        return status;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "No previous health status found, defaulting to false");
        return false;
    } else {
        ESP_LOGE(TAG, "Error reading last health status: %s", esp_err_to_name(err));
        return false;
    }
}
