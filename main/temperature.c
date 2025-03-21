#include "temperature.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <onewire_bus.h>
#include <ds18b20.h>

#define TAG "temperature"

static ds18b20_device_handle_t thermometer_device;
static onewire_device_t next_onewire_device;

float current_temperature;

void init_temperature_sensor(int p_pin) {
    onewire_bus_handle_t bus;
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = p_pin,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));

    onewire_device_iter_handle_t iter = NULL;
    esp_err_t search_result = ESP_OK;

    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    ESP_LOGI(TAG, "Device iterator created, start searching...");

    search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
    if (search_result == ESP_OK) {
        ds18b20_config_t ds_cfg = {};
        if (ds18b20_new_device(&next_onewire_device, &ds_cfg, &thermometer_device) == ESP_OK) {
            ESP_LOGI(TAG,
                     "Found a DS18B20, address: %016llX",
                     (unsigned long long)next_onewire_device.address);
        } else {
            ESP_LOGI(TAG,
                     "Found an unknown device, address: %016llX",
                     (unsigned long long)next_onewire_device.address);
        }
    }

    ESP_ERROR_CHECK(onewire_del_device_iter(iter));

    ESP_ERROR_CHECK(ds18b20_set_resolution(thermometer_device, DS18B20_RESOLUTION_12B));

    ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(thermometer_device));
    ESP_ERROR_CHECK(ds18b20_get_temperature(thermometer_device, &current_temperature));
}

void temperature_read_loop(temperature_read_cb_t p_cb) {
    while (true) {
        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(thermometer_device));
        ESP_ERROR_CHECK(ds18b20_get_temperature(thermometer_device, &current_temperature));
        p_cb(current_temperature);

        vTaskDelay(1);
    }
}
