#pragma once
#include <esp_http_server.h>

#define FD_EVERYONE -1

void load_web_pages();
httpd_handle_t setup_web_server();
esp_err_t send_temperature_update(float p_temperature, int p_target);
esp_err_t send_target_temperature_update(int p_target);
esp_err_t send_heater_state_update(int p_target);
