#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern float target_temperature;
extern bool is_heating_on;
extern SemaphoreHandle_t configuration_mutex;

void init_heater(int p_pin);
