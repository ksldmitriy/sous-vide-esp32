#include "web_site.h"
#include "heater.h"

#include <esp_spiffs.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <sys/stat.h>
#include <cJSON.h>

#define TAG "web site"

static char *s_index_html;
static char *s_styles_css;
static char *s_layout_css;
static char *s_d3_js;
static char *s_main_js;

struct {
    const char *path;
    char **var;
} static s_files[4] = {{"/spiffs/index.html", &s_index_html},
                       {"/spiffs/styles.css", &s_styles_css},
                       {"/spiffs/layout.css", &s_layout_css},
                       {"/spiffs/d3.js", &s_d3_js},
                       {"/spiffs/main.js", &s_main_js}};

static httpd_handle_t s_server = NULL;

static esp_err_t get_req_handler(httpd_req_t *p_req);
static esp_err_t ws_req_handler(httpd_req_t *p_req);

static esp_err_t on_message(httpd_ws_frame_t p_frame);

struct {
    char *text;
    int target;
} typedef ws_message_t;

static void ws_async_send(ws_message_t *p_message);

void load_web_pages() {
    esp_vfs_spiffs_conf_t conf = {.base_path = "/spiffs",
                                  .partition_label = NULL,
                                  .max_files = 5,
                                  .format_if_mount_failed = true};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    for (int i = 0; i < sizeof(s_files) / sizeof(*s_files); i++) {
        struct stat st;
        if (stat(s_files[i].path, &st)) {
            ESP_LOGE(TAG, "file \"%s\" not found", s_files[i].path);
            return;
        }

        *s_files[i].var = calloc(st.st_size + 1, 1);
        if (*s_files[i].var == NULL) {
            ESP_LOGE(TAG, "cant allocate memory for %s", s_files[i].path);
        }

        FILE *fp = fopen(s_files[i].path, "r");
        if (fread(*s_files[i].var, st.st_size, 1, fp) == 0) {
            ESP_LOGE(TAG, "fread failed for file \"%s\"", s_files[i].path);
        }
        fclose(fp);
    }
}

httpd_handle_t setup_web_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&s_server, &config) != ESP_OK) {
        return s_server;
    }

    httpd_uri_t uri_get_file = {.method = HTTP_GET, .handler = get_req_handler, .user_ctx = NULL};
    uri_get_file.uri = "/", httpd_register_uri_handler(s_server, &uri_get_file);

    uri_get_file.uri = "/index.html";
    httpd_register_uri_handler(s_server, &uri_get_file);

    uri_get_file.uri = "/styles.css";
    httpd_register_uri_handler(s_server, &uri_get_file);

    uri_get_file.uri = "/layout.css";
    httpd_register_uri_handler(s_server, &uri_get_file);

    uri_get_file.uri = "/d3.js";
    httpd_register_uri_handler(s_server, &uri_get_file);

    uri_get_file.uri = "/main.js";
    httpd_register_uri_handler(s_server, &uri_get_file);

    httpd_uri_t ws = {.uri = "/ws",
                      .method = HTTP_GET,
                      .handler = ws_req_handler,
                      .user_ctx = NULL,
                      .is_websocket = true};
    httpd_register_uri_handler(s_server, &ws);

    configuration_mutex = xSemaphoreCreateMutex();
    if (!configuration_mutex) {
        ESP_LOGE(TAG, "failed to create target_temperature_mutex");
        esp_restart();
    }

    ESP_LOGI(TAG, "server started");

    return s_server;
}

esp_err_t send_temperature_update(float p_temperature, int p_target) {
    const char *const fmt_str = "{ \"current_temperature\":%f}";

    ws_message_t *message = malloc(sizeof(ws_message_t));
    message->target = FD_EVERYONE;

    int req_size = snprintf(NULL, 0, fmt_str, p_temperature);
    message->text = malloc(req_size + 1);
    snprintf(message->text, req_size + 1, fmt_str, p_temperature);

    return httpd_queue_work(s_server, (httpd_work_fn_t)ws_async_send, message);
}

esp_err_t send_target_temperature_update(int p_target) {
    const char *const fmt_str = "{ \"target_temperature\":%f}";

    ws_message_t *message = malloc(sizeof(ws_message_t));
    message->target = FD_EVERYONE;

    if (xSemaphoreTake(configuration_mutex, portMAX_DELAY) == pdTRUE) {
        int req_size = snprintf(NULL, 0, fmt_str, target_temperature);
        message->text = malloc(req_size + 1);
        snprintf(message->text, req_size + 1, fmt_str, target_temperature);

        xSemaphoreGive(configuration_mutex);
    } else {
        ESP_LOGE(TAG, "failed to take target_temperature_mutex");
        esp_restart();
    }

    return httpd_queue_work(s_server, (httpd_work_fn_t)ws_async_send, message);
}

static esp_err_t get_req_handler(httpd_req_t *p_req) {
    ESP_LOGI(TAG, "request to %s", p_req->uri);

    esp_err_t ret = ESP_FAIL;
    const char *data = NULL;
    if (!strcmp(p_req->uri, "/index.html") || !strcmp(p_req->uri, "/")) {
        ret = httpd_resp_set_type(p_req, "text/html");
        data = s_index_html;
    } else if (!strcmp(p_req->uri, "/styles.css")) {
        ret = httpd_resp_set_type(p_req, "text/css");
        data = s_styles_css;
    } else if (!strcmp(p_req->uri, "/layout.css")) {
        ret = httpd_resp_set_type(p_req, "text/css");
        data = s_layout_css;
    } else if (!strcmp(p_req->uri, "/d3.js")) {
        ret = httpd_resp_set_type(p_req, "text/javascript");
        data = s_d3_js;
    } else if (!strcmp(p_req->uri, "/main.js")) {
        ret = httpd_resp_set_type(p_req, "text/javascript");
        data = s_main_js;
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_resp_set_type failed with %s", esp_err_to_name(ret));
        return ret;
    }

    if (data == NULL) {
        ret = httpd_resp_send(p_req, "Invalid URI", HTTPD_RESP_USE_STRLEN);
    } else {
        ret = httpd_resp_send(p_req, data, HTTPD_RESP_USE_STRLEN);
    }

    return ret;
}

static esp_err_t ws_req_handler(httpd_req_t *p_req) {
    if (p_req->method == HTTP_GET) { // handshake done
        send_target_temperature_update(httpd_req_to_sockfd(p_req));
        return ESP_OK;
    }

    httpd_ws_frame_t frame;
    uint8_t *buf = NULL;
    memset(&frame, 0, sizeof(httpd_ws_frame_t));
    frame.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(p_req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %s", esp_err_to_name(ret));
        return ret;
    } else if (frame.len == 0) {
        ESP_LOGI(TAG, "received empty frame");
        return ESP_OK;
    }

    buf = calloc(1, frame.len + 1);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to calloc memory for buf");
        return ESP_ERR_NO_MEM;
    }

    frame.payload = buf;
    ret = httpd_ws_recv_frame(p_req, &frame, frame.len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %s", esp_err_to_name(ret));
        free(buf);
        return ret;
    }

    return on_message(frame);
}

static esp_err_t on_message(httpd_ws_frame_t p_frame) {
    cJSON *root = cJSON_Parse((const char *)p_frame.payload);
    if (!root) {
        ESP_LOGE(TAG, "invalid json received");
        return -1;
    }

    if (xSemaphoreTake(configuration_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "failed to take target_temperature_mutex");
        esp_restart();
    }

    cJSON *target_temperature_json = cJSON_GetObjectItem(root, "target_temperature");
    if (target_temperature_json && target_temperature_json->type == cJSON_Number) {
        target_temperature = target_temperature_json->valuedouble;
        ESP_LOGI(TAG, "new target temperature %f", target_temperature);
        xSemaphoreGive(configuration_mutex);
        send_target_temperature_update(FD_EVERYONE);
    }

    cJSON *is_heating_on_json = cJSON_GetObjectItem(root, "is_heating_on");
    if (is_heating_on_json &&
        (is_heating_on_json->type == cJSON_True || is_heating_on_json->type == cJSON_False)) {
        is_heating_on = is_heating_on_json->type == cJSON_True;
        ESP_LOGI(TAG, "is on updated to: %s", is_heating_on ? "ON" : "OFF");
    }

    cJSON_Delete(root);

    free(p_frame.payload);
    return ESP_OK;
}

// httpd_queue_work(s_server, ws_async_send, p_message);

static void ws_async_send(ws_message_t *p_message) {

    httpd_ws_frame_t ws_pkt;
    ws_pkt.payload = (uint8_t *)p_message->text;
    ws_pkt.len = strlen(p_message->text);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    if (p_message->target == FD_EVERYONE) {
        static size_t max_clients = CONFIG_LWIP_MAX_LISTENING_TCP;
        size_t fds = max_clients;
        int client_fds[max_clients];

        esp_err_t ret = httpd_get_client_list(s_server, &fds, client_fds);
        if (ret != ESP_OK) {
            return;
        }

        for (int i = 0; i < fds; i++) {
            int client_info = httpd_ws_get_fd_info(s_server, client_fds[i]);
            if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
                httpd_ws_send_frame_async(s_server, client_fds[i], &ws_pkt);
            }
        }
    } else {
        httpd_ws_send_frame_async(s_server, p_message->target, &ws_pkt);
    }

    free(p_message->text);
    free(p_message);
}
