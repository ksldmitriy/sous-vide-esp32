#pragma once
#include <esp_err.h>

typedef void (*temperature_read_cb_t)(float p_temperature);

extern float current_temperature;

void init_temperature_sensor(int p_pin);
void temperature_read_loop(temperature_read_cb_t p_cb);
