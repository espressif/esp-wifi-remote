/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi_remote.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_BITS (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT)
#define WIFI_REMOTE_CONNECTED_BIT BIT2
#define WIFI_REMOTE_FAIL_BIT      BIT3
#define WIFI_REMOTE_BITS (WIFI_REMOTE_CONNECTED_BIT | WIFI_REMOTE_FAIL_BIT)

static int s_retry_num = 0;

#if CONFIG_ESP_WIFI_LOCAL_ENABLE
static const char *TAG_local = "two_stations_local";

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_local, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG_local,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        if (event->esp_netif != esp_netif_get_handle_from_ifkey("WIFI_STA_DEF")) {
            // Ignore this event (not the local Wi-Fi interface), probably remote Wi-Fi
            return;
        }
        ESP_LOGI(TAG_local, "Local Wi-Fi got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
#endif

#if CONFIG_ESP_WIFI_REMOTE_ENABLE
static const char *TAG_remote = "two_stations_remote";

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
#endif

static void init_system_components(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

#if CONFIG_ESP_WIFI_LOCAL_ENABLE
static void wifi_init_sta(void)
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_LOCAL_SSID,
            .password = CONFIG_ESP_WIFI_LOCAL_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG_local, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_BITS, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_local, "connected to ap SSID:%s ", CONFIG_ESP_WIFI_LOCAL_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG_local, "Failed to connect to SSID:%s", CONFIG_ESP_WIFI_LOCAL_SSID);
    } else {
        ESP_LOGE(TAG_local, "UNEXPECTED EVENT");
    }
}
#endif

#if CONFIG_ESP_WIFI_REMOTE_ENABLE
static void wifi_init_remote_sta(void)
{
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
#endif

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_system_components();

#if CONFIG_ESP_WIFI_LOCAL_ENABLE
    wifi_init_sta();
#endif

#if CONFIG_ESP_WIFI_REMOTE_ENABLE
    wifi_init_remote_sta();
#endif
}
