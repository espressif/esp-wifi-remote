/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
// It is important to include *injected* headers before *esp-wifi-remote*
// to use **SOC_SLAVE** Wi-Fi types in API typedefs, enums, structs
#include "injected/esp_wifi.h"
#include "esp_wifi_remote.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_mac.h"

static const char *TAG = "two_stations_remote_ap";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

void wifi_init_remote_ap(void)
{
    esp_wifi_remote_create_default_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_remote_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_REMOTE_SSID,
            .ssid_len = strlen(CONFIG_ESP_WIFI_REMOTE_SSID),
            .password = CONFIG_ESP_WIFI_REMOTE_PASSWORD,
            .max_connection = CONFIG_ESP_WIFI_REMOTE_MAX_STATIONS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_remote_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_remote_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_remote_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s", CONFIG_ESP_WIFI_REMOTE_SSID);
}
