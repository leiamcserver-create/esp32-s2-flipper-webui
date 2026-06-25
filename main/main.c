#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/uart.h"

#define UART_NUM UART_NUM_1
#define TXD_PIN 17
#define RXD_PIN 16

static const char *SSID = "ESP32_CONTROL";
static const char *PASS = "12345678";

// ---------------- UART ----------------
void uart_init() {
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM, &cfg);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0);
}

void send_to_pico(const char *msg) {
    uart_write_bytes(UART_NUM, msg, strlen(msg));
    uart_write_bytes(UART_NUM, "\n", 1);
}

// ---------------- WEB HANDLERS ----------------
esp_err_t root_get_handler(httpd_req_t *req) {
    const char html[] =
        "<h1>ESP32 Control Panel</h1>"
        "<button onclick=\"fetch('/led/on')\">LED ON</button>"
        "<button onclick=\"fetch('/led/off')\">LED OFF</button>"
        "<button onclick=\"fetch('/pico/ping')\">Ping Pico</button>";

    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t led_on_handler(httpd_req_t *req) {
    send_to_pico("LED ON");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

esp_err_t led_off_handler(httpd_req_t *req) {
    send_to_pico("LED OFF");
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

esp_err_t pico_ping_handler(httpd_req_t *req) {
    send_to_pico("PING");
    httpd_resp_send(req, "PING SENT", 9);
    return ESP_OK;
}

// ---------------- SERVER ----------------
httpd_uri_t uri_root = {.uri="/", .method=HTTP_GET, .handler=root_get_handler};
httpd_uri_t uri_on   = {.uri="/led/on", .method=HTTP_GET, .handler=led_on_handler};
httpd_uri_t uri_off  = {.uri="/led/off", .method=HTTP_GET, .handler=led_off_handler};
httpd_uri_t uri_ping = {.uri="/pico/ping", .method=HTTP_GET, .handler=pico_ping_handler};

httpd_handle_t start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_start(&server, &config);

    httpd_register_uri_handler(server, &uri_root);
    httpd_register_uri_handler(server, &uri_on);
    httpd_register_uri_handler(server, &uri_off);
    httpd_register_uri_handler(server, &uri_ping);

    return server;
}

// ---------------- WIFI AP ----------------
void wifi_init_softap() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP32_CONTROL",
            .ssid_len = strlen("ESP32_CONTROL"),
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
}

// ---------------- MAIN ----------------
void app_main() {
    nvs_flash_init();

    uart_init();
    wifi_init_softap();
    start_server();
}
