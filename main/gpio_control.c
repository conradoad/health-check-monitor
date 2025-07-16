#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "config.h"
#include "gpio_control.h"

static const char *TAG = "GPIO_CONTROL";

void gpio_control_init(void)
{
    ESP_LOGI(TAG, "Initializing GPIO control");
    
    // Configure button as input with internal pull-up
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << GPIO_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&button_config);
    
    // Configure relay as output
    gpio_config_t relay_config = {
        .pin_bit_mask = (1ULL << GPIO_RELAY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&relay_config);
    
    // Configure blue LED as output
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << GPIO_BLUE_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_config);
    
    // Initialize outputs to OFF
    gpio_set_level(GPIO_RELAY, 0);
    gpio_set_level(GPIO_BLUE_LED, 0);
    
    ESP_LOGI(TAG, "GPIO control initialized");
    ESP_LOGI(TAG, "Button: GPIO%d (input, pull-up)", GPIO_BUTTON);
    ESP_LOGI(TAG, "Relay: GPIO%d (output)", GPIO_RELAY);
    ESP_LOGI(TAG, "Blue LED: GPIO%d (output)", GPIO_BLUE_LED);
}

void gpio_control_set_relay(bool state)
{
    gpio_set_level(GPIO_RELAY, state ? 1 : 0);
    ESP_LOGI(TAG, "Relay %s", state ? "ON" : "OFF");
}

void gpio_control_set_blue_led(bool state)
{
    gpio_set_level(GPIO_BLUE_LED, state ? 1 : 0);
    ESP_LOGD(TAG, "Blue LED %s", state ? "ON" : "OFF");
}

bool gpio_control_get_button_state(void)
{
    // Button is active LOW (pressed = 0)
    return (gpio_get_level(GPIO_BUTTON) == 0);
}
