#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "config.h"
#include "config_server.h"

static const char *TAG = "CONFIG_SERVER";

// External functions
extern void save_device_config(void);
extern void switch_to_execution_mode(void);
extern device_config_t* get_device_config(void);

// Global variables
static httpd_handle_t server = NULL;

// Function prototypes
static esp_err_t config_get_handler(httpd_req_t *req);
static esp_err_t config_post_handler(httpd_req_t *req);
static esp_err_t status_get_handler(httpd_req_t *req);
static esp_err_t root_get_handler(httpd_req_t *req);

// Task for switching to execution mode
static void switch_mode_task(void* pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
    switch_to_execution_mode();
    vTaskDelete(NULL);
}

// HTML page for configuration
static const char* config_html = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"    <title>SONOFF Monitor Configuration</title>"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"    <style>"
"        body { font-family: Arial, sans-serif; margin: 20px; }"
"        .container { max-width: 500px; margin: 0 auto; }"
"        .form-group { margin-bottom: 15px; }"
"        label { display: block; margin-bottom: 5px; font-weight: bold; }"
"        input[type=\"text\"], input[type=\"password\"], input[type=\"url\"], input[type=\"number\"] {"
"            width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box;"
"        }"
"        button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; }"
"        button:hover { background-color: #45a049; }"
"        .status { margin-top: 20px; padding: 10px; border-radius: 4px; }"
"        .success { background-color: #d4edda; color: #155724; }"
"        .error { background-color: #f8d7da; color: #721c24; }"
"    </style>"
"</head>"
"<body>"
"    <div class=\"container\">"
"        <h1>SONOFF Monitor Configuration</h1>"
"        <form id=\"configForm\">"
"            <div class=\"form-group\">"
"                <label for=\"wifi_ssid\">WiFi SSID:</label>"
"                <input type=\"text\" id=\"wifi_ssid\" name=\"wifi_ssid\" required>"
"            </div>"
"            <div class=\"form-group\">"
"                <label for=\"wifi_password\">WiFi Password:</label>"
"                <input type=\"password\" id=\"wifi_password\" name=\"wifi_password\" required>"
"            </div>"
"            <div class=\"form-group\">"
"                <label for=\"health_check_url\">Health Check URL:</label>"
"                <input type=\"url\" id=\"health_check_url\" name=\"health_check_url\" required placeholder=\"http://example.com/health\">"
"            </div>"
"            <div class=\"form-group\">"
"                <label for=\"check_interval\">Check Interval (seconds):</label>"
"                <input type=\"number\" id=\"check_interval\" name=\"check_interval\" min=\"10\" max=\"3600\" value=\"30\" required>"
"            </div>"
"            <button type=\"submit\">Save Configuration</button>"
"        </form>"
"        <div id=\"status\"></div>"
"    </div>"
"    <script>"
"        document.getElementById('configForm').addEventListener('submit', function(e) {"
"            e.preventDefault();"
"            const formData = new FormData(e.target);"
"            const data = {"
"                wifi_ssid: formData.get('wifi_ssid'),"
"                wifi_password: formData.get('wifi_password'),"
"                health_check_url: formData.get('health_check_url'),"
"                check_interval: parseInt(formData.get('check_interval')) * 1000"
"            };"
"            fetch('/config', {"
"                method: 'POST',"
"                headers: { 'Content-Type': 'application/json' },"
"                body: JSON.stringify(data)"
"            })"
"            .then(response => response.json())"
"            .then(data => {"
"                const status = document.getElementById('status');"
"                if (data.success) {"
"                    status.className = 'status success';"
"                    status.textContent = 'Configuration saved successfully! Device will restart in execution mode.';"
"                    setTimeout(() => { window.location.reload(); }, 3000);"
"                } else {"
"                    status.className = 'status error';"
"                    status.textContent = 'Error: ' + (data.message || 'Unknown error');"
"                }"
"            })"
"            .catch(error => {"
"                const status = document.getElementById('status');"
"                status.className = 'status error';"
"                status.textContent = 'Error: ' + error.message;"
"            });"
"        });"
"    </script>"
"</body>"
"</html>";

void config_server_start(void)
{
    ESP_LOGI(TAG, "Starting configuration server");
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = HTTP_SERVER_PORT;
    config.lru_purge_enable = true;
    
    // Start the httpd server
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "HTTP server started on port %d", HTTP_SERVER_PORT);
        
        // Register URI handlers
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        
        httpd_uri_t config_get_uri = {
            .uri = "/config",
            .method = HTTP_GET,
            .handler = config_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &config_get_uri);
        
        httpd_uri_t config_post_uri = {
            .uri = "/config",
            .method = HTTP_POST,
            .handler = config_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &config_post_uri);
        
        httpd_uri_t status_uri = {
            .uri = "/status",
            .method = HTTP_GET,
            .handler = status_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &status_uri);
        
        ESP_LOGI(TAG, "Configuration server started successfully");
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}

void config_server_stop(void)
{
    if (server) {
        ESP_LOGI(TAG, "Stopping configuration server");
        httpd_stop(server);
        server = NULL;
    }
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving root page");
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, config_html, strlen(config_html));
    
    return ESP_OK;
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /config request");
    
    device_config_t* config = get_device_config();
    
    cJSON *json = cJSON_CreateObject();
    cJSON *wifi_ssid = cJSON_CreateString(config->wifi_ssid);
    cJSON *health_check_url = cJSON_CreateString(config->health_check_url);
    cJSON *check_interval = cJSON_CreateNumber(config->check_interval_ms / 1000);
    cJSON *configured = cJSON_CreateBool(config->configured);
    
    cJSON_AddItemToObject(json, "wifi_ssid", wifi_ssid);
    cJSON_AddItemToObject(json, "health_check_url", health_check_url);
    cJSON_AddItemToObject(json, "check_interval", check_interval);
    cJSON_AddItemToObject(json, "configured", configured);
    
    char *json_string = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /config request");
    
    char content[1024];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    
    content[ret] = '\0';
    ESP_LOGI(TAG, "Received configuration: %s", content);
    
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "Invalid JSON");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    device_config_t* config = get_device_config();
    cJSON *response = cJSON_CreateObject();
    bool success = true;
    
    // Parse WiFi SSID
    cJSON *wifi_ssid = cJSON_GetObjectItem(json, "wifi_ssid");
    if (cJSON_IsString(wifi_ssid) && (wifi_ssid->valuestring != NULL)) {
        strncpy(config->wifi_ssid, wifi_ssid->valuestring, sizeof(config->wifi_ssid) - 1);
        config->wifi_ssid[sizeof(config->wifi_ssid) - 1] = '\0';
    } else {
        success = false;
        ESP_LOGE(TAG, "Invalid or missing wifi_ssid");
    }
    
    // Parse WiFi Password
    cJSON *wifi_password = cJSON_GetObjectItem(json, "wifi_password");
    if (cJSON_IsString(wifi_password) && (wifi_password->valuestring != NULL)) {
        strncpy(config->wifi_password, wifi_password->valuestring, sizeof(config->wifi_password) - 1);
        config->wifi_password[sizeof(config->wifi_password) - 1] = '\0';
    } else {
        success = false;
        ESP_LOGE(TAG, "Invalid or missing wifi_password");
    }
    
    // Parse Health Check URL
    cJSON *health_check_url = cJSON_GetObjectItem(json, "health_check_url");
    if (cJSON_IsString(health_check_url) && (health_check_url->valuestring != NULL)) {
        strncpy(config->health_check_url, health_check_url->valuestring, sizeof(config->health_check_url) - 1);
        config->health_check_url[sizeof(config->health_check_url) - 1] = '\0';
    } else {
        success = false;
        ESP_LOGE(TAG, "Invalid or missing health_check_url");
    }
    
    // Parse Check Interval
    cJSON *check_interval = cJSON_GetObjectItem(json, "check_interval");
    if (cJSON_IsNumber(check_interval)) {
        config->check_interval_ms = (uint32_t)check_interval->valueint;
        if (config->check_interval_ms < 10000) {
            config->check_interval_ms = 10000; // Minimum 10 seconds
        }
    } else {
        success = false;
        ESP_LOGE(TAG, "Invalid or missing check_interval");
    }
    
    if (success) {
        config->configured = true;
        
        // Save configuration
        save_device_config();
        
        cJSON_AddTrueToObject(response, "success");
        cJSON_AddStringToObject(response, "message", "Configuration saved successfully");
        
        ESP_LOGI(TAG, "Configuration saved successfully");
        ESP_LOGI(TAG, "WiFi SSID: %s", config->wifi_ssid);
        ESP_LOGI(TAG, "Health URL: %s", config->health_check_url);
        ESP_LOGI(TAG, "Check interval: %d ms", config->check_interval_ms);
        
        // Schedule mode switch after response
        xTaskCreate(switch_mode_task, "switch_mode", 2048, NULL, 5, NULL);
        
    } else {
        cJSON_AddFalseToObject(response, "success");
        cJSON_AddStringToObject(response, "message", "Invalid configuration parameters");
    }
    
    char *response_string = cJSON_Print(response);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_string, strlen(response_string));
    
    free(response_string);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /status request");
    
    device_config_t* config = get_device_config();
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "mode", "configuration");
    cJSON_AddBoolToObject(json, "configured", config->configured);
    cJSON_AddStringToObject(json, "version", "1.0.0");
    cJSON_AddStringToObject(json, "device", "SONOFF MINI");
    
    char *json_string = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}
