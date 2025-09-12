/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "console_ping.h"
#include "iperf_cmd.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_BITS (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT)
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

#if CONFIG_ESP_WIFI_REMOTE_ENABLE
/* Remote Wi-Fi is defined in another compilation unit,
 * as wifi_remote API uses the same type names as local WiFi
 * and since the types might be slightly different for different
 * targets, we cannot combine both in a single compilation unit
 */
void wifi_init_remote_sta(void);
#endif

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

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
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

    // at this point, we should be connected (via at least one station)
    ESP_ERROR_CHECK(console_cmd_init());
    // now let's register ping and iperf utilities to the console
    app_register_iperf_commands();

    ESP_ERROR_CHECK(console_cmd_ping_register());
    ESP_ERROR_CHECK(console_cmd_start());

}
