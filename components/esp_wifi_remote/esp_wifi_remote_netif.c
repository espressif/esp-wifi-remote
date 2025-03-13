/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_private/wifi.h"
#include "esp_wifi_netif.h"
#include <string.h>
#include <inttypes.h>

/**
 * This is a dummy driver, since we support only STA and AP interfaces
 * no need to implement the driver, only assure that the handle is not NULL
 */
#define WIFI_REMOTE_DRIVER_HANDLE ((void*)1)

#define IMPLEMENT_WIFI_TRANSMIT(name, interface) static esp_err_t name(void *h, void *buffer, size_t len) \
                                                 { return esp_wifi_internal_tx(interface, buffer, len); }
#define IMPLEMENT_WIFI_TRANSMIT_WRAP(name, interface) static esp_err_t name(void *h, void *buffer, size_t len, void *netbuf) \
                                                 { return esp_wifi_internal_tx(interface, buffer, len); }

extern const esp_netif_netstack_config_t *_g_esp_netif_netstack_default_wifi_ap;
extern const esp_netif_netstack_config_t *_g_esp_netif_netstack_default_wifi_sta;

static bool wifi_default_handlers_set = false;
static esp_netif_t *s_wifi_netifs[MAX_WIFI_IFS] = { NULL };
static const char* TAG = "wifi_remote_default_netif";

IMPLEMENT_WIFI_TRANSMIT(transmit_sta, WIFI_IF_STA)
IMPLEMENT_WIFI_TRANSMIT(transmit_ap, WIFI_IF_AP)
IMPLEMENT_WIFI_TRANSMIT_WRAP(transmit_wrap_sta, WIFI_IF_STA)
IMPLEMENT_WIFI_TRANSMIT_WRAP(transmit_wrap_ap, WIFI_IF_AP)

static void wifi_free(void *h, void* buffer)
{
    if (buffer) {
        esp_wifi_internal_free_rx_buffer(buffer);
    }
}

static esp_err_t receive_sta(void *buffer, uint16_t len, void *eb)
{
    return esp_netif_receive(s_wifi_netifs[WIFI_IF_STA], buffer, len, eb);
}

static esp_err_t receive_ap(void *buffer, uint16_t len, void *eb)
{
    return esp_netif_receive(s_wifi_netifs[WIFI_IF_AP], buffer, len, eb);
}

/**
 * @brief Wifi start action when station or AP get started
 */
static void wifi_start(wifi_interface_t ifx, esp_event_base_t base, int32_t event_id, void *data)
{
    uint8_t mac[6];
    esp_err_t ret;

    if ((ret = esp_wifi_get_mac(ifx, mac)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_get_mac failed with %d", ret);
        return;
    }
    ESP_LOGD(TAG, "WIFI mac address: %x %x %x %x %x %x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if (ifx == WIFI_IF_AP) {
        if ((ret = esp_wifi_internal_reg_rxcb(ifx,  receive_ap)) != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_internal_reg_rxcb for if=%d failed with %d", ifx, ret);
            return;
        }
    }

    if ((ret = esp_wifi_internal_reg_netstack_buf_cb(esp_netif_netstack_buf_ref, esp_netif_netstack_buf_free)) != ESP_OK) {
        ESP_LOGE(TAG, "netstack cb reg failed with %d", ret);
        return;
    }
    esp_netif_set_mac(s_wifi_netifs[ifx], mac);
    esp_netif_action_start(s_wifi_netifs[ifx], base, event_id, data);
}

/**
 * @brief Wifi default handlers for specific events for station and APs
 */

static void wifi_default_action_sta_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    if (s_wifi_netifs[WIFI_IF_STA] != NULL) {
        wifi_start(WIFI_IF_STA, base, event_id, data);
    }
}

static void wifi_default_action_sta_stop(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    if (s_wifi_netifs[WIFI_IF_STA] != NULL) {
        esp_netif_action_stop(s_wifi_netifs[WIFI_IF_STA], base, event_id, data);
    }
}

static void wifi_default_action_sta_connected(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    if (s_wifi_netifs[WIFI_IF_STA] != NULL) {
        esp_err_t ret;
        if ((ret = esp_wifi_internal_reg_rxcb(WIFI_IF_STA,  receive_sta)) != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_internal_reg_rxcb for if=%d failed with %d", WIFI_IF_STA, ret);
            return;
        }
        esp_netif_action_connected(s_wifi_netifs[WIFI_IF_STA], base, event_id, data);
    }
}

static void wifi_default_action_sta_disconnected(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    if (s_wifi_netifs[WIFI_IF_STA] != NULL) {
        esp_netif_action_disconnected(s_wifi_netifs[WIFI_IF_STA], base, event_id, data);
    }
}

#ifdef CONFIG_WIFI_RMT_SOFTAP_SUPPORT
static void wifi_default_action_ap_start(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    if (s_wifi_netifs[WIFI_IF_AP] != NULL) {
        wifi_start(WIFI_IF_AP, base, event_id, data);
    }
}

static void wifi_default_action_ap_stop(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    if (s_wifi_netifs[WIFI_IF_AP] != NULL) {
        esp_netif_action_stop(s_wifi_netifs[WIFI_IF_AP], base, event_id, data);
    }
}
#endif

static void wifi_default_action_sta_got_ip(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    if (s_wifi_netifs[WIFI_IF_STA] != NULL) {
        ESP_LOGD(TAG, "Got IP wifi default handler entered");
        int ret = esp_wifi_internal_set_sta_ip();
        if (ret != ESP_OK) {
            ESP_LOGI(TAG, "esp_wifi_internal_set_sta_ip failed with %d", ret);
        }
        esp_netif_action_got_ip(s_wifi_netifs[WIFI_IF_STA], base, event_id, data);
    }
}
/**
 * @brief Clear default handlers
 */
static esp_err_t clear_default_wifi_handlers(void)
{
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_START, wifi_default_action_sta_start);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_STOP, wifi_default_action_sta_stop);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, wifi_default_action_sta_connected);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_default_action_sta_disconnected);
#ifdef CONFIG_WIFI_RMT_SOFTAP_SUPPORT
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_START, wifi_default_action_ap_start);
    esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STOP, wifi_default_action_ap_stop);
#endif
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_default_action_sta_got_ip);
    esp_unregister_shutdown_handler((shutdown_handler_t)esp_wifi_stop);
    wifi_default_handlers_set = false;
    return ESP_OK;
}
/**
 * @brief Set default handlers
 */
static esp_err_t set_default_wifi_handlers(void)
{
    if (wifi_default_handlers_set) {
        return ESP_OK;
    }
    esp_err_t err;

    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, wifi_default_action_sta_start, NULL);
    if (err != ESP_OK) {
        goto fail;
    }

    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, wifi_default_action_sta_stop, NULL);
    if (err != ESP_OK) {
        goto fail;
    }

    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, wifi_default_action_sta_connected, NULL);
    if (err != ESP_OK) {
        goto fail;
    }

    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_default_action_sta_disconnected, NULL);
    if (err != ESP_OK) {
        goto fail;
    }

#ifdef CONFIG_WIFI_RMT_SOFTAP_SUPPORT
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, wifi_default_action_ap_start, NULL);
    if (err != ESP_OK) {
        goto fail;
    }

    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, wifi_default_action_ap_stop, NULL);
    if (err != ESP_OK) {
        goto fail;
    }
#endif

    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_default_action_sta_got_ip, NULL);
    if (err != ESP_OK) {
        goto fail;
    }


    err = esp_register_shutdown_handler((shutdown_handler_t)esp_wifi_stop);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        goto fail;
    }
    wifi_default_handlers_set = true;
    return ESP_OK;

    fail:
    clear_default_wifi_handlers();
    return err;
}

/**
 * @brief User init custom wifi interface
 */
esp_netif_t* esp_netif_create_wifi(wifi_interface_t wifi_if, const esp_netif_inherent_config_t *esp_netif_config) {
    esp_netif_driver_ifconfig_t driver_ifconfig = {
            .handle =  WIFI_REMOTE_DRIVER_HANDLE,
            .driver_free_rx_buffer = wifi_free
    };
    esp_netif_config_t cfg = {
            .base = esp_netif_config,
            .driver = &driver_ifconfig
    };
    if (wifi_if == WIFI_IF_STA) {
        cfg.stack = _g_esp_netif_netstack_default_wifi_sta;
        driver_ifconfig.transmit = transmit_sta;
        driver_ifconfig.transmit_wrap = transmit_wrap_sta;
    } else if (wifi_if == WIFI_IF_AP) {
        cfg.stack = _g_esp_netif_netstack_default_wifi_ap;
        driver_ifconfig.transmit = transmit_ap;
        driver_ifconfig.transmit_wrap = transmit_wrap_ap;
    } else {
        return NULL;
    }

    esp_netif_t *netif = esp_netif_new(&cfg);
    assert(netif);
    s_wifi_netifs[wifi_if] = netif;
    return netif;
}

/**
 * @brief Set default handlers for station (official API)
 */
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void)
{
    return set_default_wifi_handlers();
}

/**
 * @brief Set default handlers for AP (official API)
 */
esp_err_t esp_wifi_set_default_wifi_ap_handlers(void)
{
    return set_default_wifi_handlers();
}

/**
 * @brief Clear default handlers and destroy appropriate objects (official API)
 */
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *esp_netif)
{
    int i;
    wifi_interface_t ifx;
    for (i = 0; i < MAX_WIFI_IFS; ++i) {
        // clear internal static pointers to netifs
        if (s_wifi_netifs[i] == esp_netif) {
            s_wifi_netifs[i] = NULL;
            ifx = i;
        }
    }
    for (i = 0; i < MAX_WIFI_IFS; ++i) {
        // check if all netifs are cleared to delete default handlers
        if (s_wifi_netifs[i] != NULL) {
            break;
        }
    }

    if (i == MAX_WIFI_IFS) { // if all wifi default netifs are null
        ESP_LOGD(TAG, "Clearing wifi default handlers");
        clear_default_wifi_handlers();
    }
    esp_wifi_internal_reg_rxcb(ifx, NULL);
    return ESP_OK;
}

#ifdef CONFIG_WIFI_RMT_SOFTAP_SUPPORT

static const esp_netif_ip_info_t s_wifi_remote_soft_ap_ip = {
        .ip = { .addr = ESP_IP4TOADDR( 192, 168, 4, 1) },
        .gw = { .addr = ESP_IP4TOADDR( 192, 168, 4, 1) },
        .netmask = { .addr = ESP_IP4TOADDR( 255, 255, 255, 0) },
};

#ifdef CONFIG_LWIP_IPV4
#define ESP_NETIF_IPV4_ONLY_FLAGS(flags) (flags)
#else
#define ESP_NETIF_IPV4_ONLY_FLAGS(flags) (0)
#endif

#define ESP_NETIF_INHERENT_DEFAULT_WIFI_AP() \
    {   \
        .flags = (esp_netif_flags_t)(ESP_NETIF_IPV4_ONLY_FLAGS(ESP_NETIF_DHCP_SERVER) | ESP_NETIF_FLAG_AUTOUP), \
        ESP_COMPILER_DESIGNATED_INIT_AGGREGATE_TYPE_EMPTY(mac) \
        .ip_info = &s_wifi_remote_soft_ap_ip, \
        .get_ip_event = 0, \
        .lost_ip_event = 0, \
        .if_key = "WIFI_AP_DEF", \
        .if_desc = "ap", \
        .route_prio = 10, \
        .bridge_info = NULL \
    }

/**
 * @brief User init default AP (official API)
 */
esp_netif_t* esp_netif_create_default_wifi_ap(void)
{
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_AP();
    esp_netif_t *netif= esp_netif_create_wifi(WIFI_IF_AP, &esp_netif_config);
    esp_wifi_set_default_wifi_ap_handlers();
    return netif;
}

/**
 * @brief User init default station (official API)
 */
esp_netif_t* esp_netif_create_default_wifi_sta(void)
{
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    esp_netif_t *netif= esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    esp_wifi_set_default_wifi_sta_handlers();
    return netif;
}

#endif
