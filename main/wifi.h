#pragma once
#include <esp_err.h>

esp_err_t init_wifi();

esp_err_t connect_to_wifi(const char *p_ssid, const char *p_password);

esp_err_t disconnect_from_wifi();

esp_err_t deinit_wifi();
