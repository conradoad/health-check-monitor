#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "config.h"
#include "wifi_manager.h"
#include "config_server.h"
#include "health_checker.h"
#include "gpio_control.h"

static const char *TAG = "MAIN";

// Global variables
device_config_t g_device_config;
bool g_config_mode = false;

// Function prototypes
static void button_task(void *pvParameters);
static void load_config_from_nvs(void);
static void save_config_to_nvs(void);
static void enter_config_mode(void);
static void enter_execution_mode(void);

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Monitor Health Checker");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize GPIO
    gpio_control_init();
    
    // Load configuration from NVS
    load_config_from_nvs();
    
    // Initialize WiFi
    wifi_manager_init();
    
    // Create button monitoring task
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    
    // Check if device is configured
    if (g_device_config.configured) {
        ESP_LOGI(TAG, "Device is configured, entering execution mode");
        enter_execution_mode();
    } else {
        ESP_LOGI(TAG, "Device not configured, entering config mode");
        enter_config_mode();
    }
}

static void button_task(void *pvParameters)
{
    uint32_t button_press_start = 0;
    bool button_pressed = false;
    
    while (1) {
        int button_state = gpio_get_level(GPIO_BUTTON);
        
        if (button_state == 0 && !button_pressed) {
            // Button pressed
            button_pressed = true;
            button_press_start = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ESP_LOGI(TAG, "Button pressed");
        } else if (button_state == 1 && button_pressed) {
            // Button released
            button_pressed = false;
            uint32_t press_duration = (xTaskGetTickCount() * portTICK_PERIOD_MS) - button_press_start;
            
            if (press_duration >= BUTTON_PRESS_TIME_MS) {
                ESP_LOGI(TAG, "Long press detected (%d ms), entering config mode", press_duration);
                enter_config_mode();
            } else {
                ESP_LOGI(TAG, "Short press detected (%d ms)", press_duration);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void load_config_from_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "NVS namespace not found, using defaults");
        memset(&g_device_config, 0, sizeof(g_device_config));
        g_device_config.check_interval_ms = DEFAULT_HEALTH_CHECK_INTERVAL_MS;
        return;
    }
    
    size_t required_size = sizeof(g_device_config.wifi_ssid);
    nvs_get_str(nvs_handle, NVS_KEY_WIFI_SSID, g_device_config.wifi_ssid, &required_size);
    
    required_size = sizeof(g_device_config.wifi_password);
    nvs_get_str(nvs_handle, NVS_KEY_WIFI_PASSWORD, g_device_config.wifi_password, &required_size);
    
    required_size = sizeof(g_device_config.health_check_url);
    nvs_get_str(nvs_handle, NVS_KEY_HEALTH_URL, g_device_config.health_check_url, &required_size);
    
    required_size = sizeof(g_device_config.check_interval_ms);
    if (nvs_get_u32(nvs_handle, NVS_KEY_CHECK_INTERVAL, &g_device_config.check_interval_ms) != ESP_OK) {
        g_device_config.check_interval_ms = DEFAULT_HEALTH_CHECK_INTERVAL_MS;
    }
    
    uint8_t configured = 0;
    if (nvs_get_u8(nvs_handle, NVS_KEY_CONFIGURED, &configured) == ESP_OK) {
        g_device_config.configured = (configured == 1);
    }
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Configuration loaded from NVS");
    ESP_LOGI(TAG, "WiFi SSID: %s", g_device_config.wifi_ssid);
    ESP_LOGI(TAG, "Health URL: %s", g_device_config.health_check_url);
    ESP_LOGI(TAG, "Check interval: %d ms", g_device_config.check_interval_ms);
    ESP_LOGI(TAG, "Configured: %s", g_device_config.configured ? "Yes" : "No");
}

static void save_config_to_nvs(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return;
    }
    
    nvs_set_str(nvs_handle, NVS_KEY_WIFI_SSID, g_device_config.wifi_ssid);
    nvs_set_str(nvs_handle, NVS_KEY_WIFI_PASSWORD, g_device_config.wifi_password);
    nvs_set_str(nvs_handle, NVS_KEY_HEALTH_URL, g_device_config.health_check_url);
    nvs_set_u32(nvs_handle, NVS_KEY_CHECK_INTERVAL, g_device_config.check_interval_ms);
    nvs_set_u8(nvs_handle, NVS_KEY_CONFIGURED, g_device_config.configured ? 1 : 0);
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Configuration saved to NVS");
}

static void enter_config_mode(void)
{
    ESP_LOGI(TAG, "Entering configuration mode");
    g_config_mode = true;
    
    // Stop health checker if running
    health_checker_stop();
    
    // Turn off relay
    gpio_control_set_relay(false);
    
    // Set blue LED to indicate config mode
    gpio_control_set_blue_led(true);
    
    // Start AP mode
    wifi_manager_start_ap();
    
    // Start configuration server
    config_server_start();
}

static void enter_execution_mode(void)
{
    ESP_LOGI(TAG, "Entering execution mode");
    g_config_mode = false;
    
    // Stop configuration server
    config_server_stop();
    
    // Turn off blue LED
    gpio_control_set_blue_led(false);
    
    // Connect to WiFi
    wifi_manager_connect_sta(g_device_config.wifi_ssid, g_device_config.wifi_password);
    
    // Start health checker
    health_checker_start(g_device_config.health_check_url, g_device_config.check_interval_ms);
}

// Global functions for other modules
void save_device_config(void)
{
    save_config_to_nvs();
}

void switch_to_execution_mode(void)
{
    enter_execution_mode();
}

device_config_t* get_device_config(void)
{
    return &g_device_config;
}
