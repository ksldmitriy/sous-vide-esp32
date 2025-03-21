#include "heater.h"

float target_temperature = 50;
bool is_heating_on = false;
SemaphoreHandle_t configuration_mutex = NULL;

static int pin;

void init_heater(int p_pin) {
    pin = p_pin;
}

void heater_on_temperature_update() {
}
