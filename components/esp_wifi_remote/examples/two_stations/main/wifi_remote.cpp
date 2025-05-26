/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi_multi_remote.hpp"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_wifi_remote_api.h"

using namespace wifi_remote::esp32c5;


esp_err_t esp_wifi_remote_set_mode(wifi_mode_t mode);

esp_err_t esp_wifi_remote_set_config(wifi_interface_t interface, wifi_config_t *conf);


// static const char *TAG = "wifi_remote";

extern "C" void configure_wifi_remote(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_REMOTE_SSID,
            .password = CONFIG_ESP_WIFI_REMOTE_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_remote_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_remote_set_config(WIFI_IF_STA, &wifi_config));

}
