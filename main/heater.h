#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <nvs_flash.h>

extern float target_temperature;
extern bool heater_state;
extern SemaphoreHandle_t configuration_mutex;
extern nvs_handle_t sous_vide_nvs_handle;

void init_heater(int p_pin);
void heater_on_temperature_update();
void save_heater_configuration_to_nvs();
