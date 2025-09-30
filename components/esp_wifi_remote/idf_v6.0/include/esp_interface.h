/*
 * SPDX-FileCopyrightText: 2015-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This has been added for backward compat to support builds if IDF-v6.0
// on both GitHub and GitLab master. It will be removed after esp_wifi API
// stabilizes on the master branch
#ifndef __ESP_INTERFACE_H__
#define __ESP_INTERFACE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_IF_WIFI_STA = 0,     /**< Station interface */
    ESP_IF_WIFI_AP,          /**< Soft-AP interface */
    ESP_IF_WIFI_NAN,         /**< NAN interface */
    ESP_IF_ETH,              /**< Ethernet interface */
    ESP_IF_MAX
} esp_interface_t;

#ifdef __cplusplus
}
#endif


#endif /* __ESP_INTERFACE_TYPES_H__ */
