#include "heater.h"

#include <esp_log.h>
#include <driver/ledc.h>
#include <temperature.h>

#define TAG "heater"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define LED_PIN 2

float target_temperature;
bool heater_state;
SemaphoreHandle_t configuration_mutex = NULL;
nvs_handle_t sous_vide_nvs_handle;

static int heater_pin;
static SemaphoreHandle_t nvs_mutex = NULL;

static void commit_heater_configuration_nvs();
static void set_duty(uint16_t p_duty);
static float lerp(float a, float b, float t);

void init_heater(int p_pin) {
    heater_pin = p_pin;

    // create mutexes
    {
        configuration_mutex = xSemaphoreCreateMutex();
        if (!configuration_mutex) {
            ESP_LOGE(TAG, "failed to create target_temperature_mutex");
            esp_restart();
        }

        nvs_mutex = xSemaphoreCreateMutex();
        if (!nvs_mutex) {
            ESP_LOGE(TAG, "failed to create target_temperature_mutex");
            esp_restart();
        }
    }

    // setup pwd
    {
        // configure timer
        ledc_timer_config_t timer_config = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                            .duty_resolution = LEDC_TIMER_16_BIT,
                                            .timer_num = LEDC_TIMER_0,
                                            .freq_hz = 4,
                                            .clk_cfg = LEDC_AUTO_CLK};
        ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

        // configure channels
        ledc_channel_config_t channel_config = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                                .channel = LEDC_CHANNEL_0,
                                                .timer_sel = LEDC_TIMER_0,
                                                .intr_type = LEDC_INTR_DISABLE,
                                                .gpio_num = LED_PIN,
                                                .duty = 0,
                                                .hpoint = 0};
        ESP_ERROR_CHECK(ledc_channel_config(&channel_config));

        channel_config.channel = LEDC_CHANNEL_1;
        channel_config.gpio_num = heater_pin;
        ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
    }
}

void heater_on_temperature_update() {
    if (xSemaphoreTake(configuration_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "failed to take target_temperature_mutex");
        esp_restart();
    }

    int duty = 0;
    if (current_temperature < target_temperature && heater_state) { // target temperature reached
        float difference = target_temperature - current_temperature;
        float t = MIN(1, difference / 5.f);
        float duty_f = lerp(0.1f, 0.6f, t);
        duty = UINT16_MAX * duty_f;
    }

    duty = MIN(duty, UINT16_MAX - 1);
    ESP_LOGI(TAG, "new duty: %f", duty / (float)UINT16_MAX);

    set_duty(duty);

    xSemaphoreGive(configuration_mutex);
}

void save_heater_configuration_to_nvs() {
    if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "failed to take nvs_mutex");
        esp_restart();
    }

    nvs_set_i8(sous_vide_nvs_handle, "is_on", heater_state);
    nvs_set_i32(sous_vide_nvs_handle, "targ_temp", *(int32_t *)&target_temperature);

    xSemaphoreGive(nvs_mutex);

    TaskHandle_t task_handle;
    xTaskCreate(commit_heater_configuration_nvs, "commit_cfg_nvs", 1024 * 3, NULL, 3, &task_handle);
}

static void commit_heater_configuration_nvs() {
    if (xSemaphoreTake(nvs_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "failed to take nvs_mutex");
        esp_restart();
    }

    nvs_commit(sous_vide_nvs_handle);

    xSemaphoreGive(nvs_mutex);

    vTaskDelete(0);
}

static void set_duty(uint16_t p_duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, p_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, p_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}
