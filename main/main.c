#include "wifi.h"
#include "temperature.h"
#include "web_site.h"

#include <nvs_flash.h>
#include <esp_log.h>

#define TAG "main"

#define WIFI_SSID "407"
#define WIFI_PASSWORD "21090421"

static void temperature_read_cb(float p_temperature);

void app_main() {
    int ret;
    // init nvs
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    // init temperature
    init_temperature_sensor(23);

    // connect to wifi
    ESP_ERROR_CHECK(init_wifi());
    ret = connect_to_wifi(WIFI_SSID, WIFI_PASSWORD);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi network");
    }

    // start web server
    load_web_pages();
    setup_web_server();

    // start temp read loop
    TaskHandle_t temp_read_task;
    ret = xTaskCreate((TaskFunction_t)temperature_read_loop,
                      "temp read loop",
                      1024 * 4,
                      temperature_read_cb,
                      tskIDLE_PRIORITY,
                      &temp_read_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "failed to create temperature read loop task");
    }
}

static void temperature_read_cb(float p_temperature) {
    send_temperature_update(p_temperature, FD_EVERYONE);
}
