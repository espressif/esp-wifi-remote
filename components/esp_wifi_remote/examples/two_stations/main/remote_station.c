/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
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

#define WIFI_REMOTE_CONNECTED_BIT BIT2
#define WIFI_REMOTE_FAIL_BIT      BIT3
#define WIFI_REMOTE_BITS (WIFI_REMOTE_CONNECTED_BIT | WIFI_REMOTE_FAIL_BIT)
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

static const char *TAG_remote = "two_stations_remote";
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

static void event_handler_remote(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_REMOTE_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_remote_connect();
    } else if (event_base == WIFI_REMOTE_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_remote_connect();
            s_retry_num++;
            ESP_LOGI(TAG_remote, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_REMOTE_FAIL_BIT);
        }
        ESP_LOGI(TAG_remote,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_remote, "Remote Wi-Fi got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_REMOTE_CONNECTED_BIT);
    }
}

void wifi_init_remote_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    esp_wifi_remote_create_default_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_remote_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_REMOTE_EVENT, ESP_EVENT_ANY_ID, event_handler_remote, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler_remote, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_REMOTE_SSID,
            .password = CONFIG_ESP_WIFI_REMOTE_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_remote_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_remote_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_remote_start());

    ESP_LOGI(TAG_remote, "wifi_init_remote_sta finished.");

    /* Waiting until either the connection is established the same way for REMOTE wifi */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_REMOTE_BITS, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_REMOTE_CONNECTED_BIT) {
        ESP_LOGI(TAG_remote, "connected to ap SSID:%s", CONFIG_ESP_WIFI_REMOTE_SSID);
    } else if (bits & WIFI_REMOTE_FAIL_BIT) {
        ESP_LOGW(TAG_remote, "Failed to connect to SSID:%s", CONFIG_ESP_WIFI_REMOTE_SSID);
    } else {
        ESP_LOGE(TAG_remote, "UNEXPECTED EVENT");
    }
}
