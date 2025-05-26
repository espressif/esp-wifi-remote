/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
namespace wifi_remote {
namespace esp32c5 {
#undef CONFIG_SLAVE_SOC_WIFI_NAN_SUPPORT
#define CONFIG_SLAVE_SOC_WIFI_NAN_SUPPORT 1
#include "esp_wifi_types.h"
#include "esp_wifi_he_types.h"
#undef CONFIG_SLAVE_SOC_WIFI_NAN_SUPPORT
}
namespace esp32c6 {
#undef CONFIG_SLAVE_SOC_WIFI_NAN_SUPPORT
#define CONFIG_SLAVE_SOC_WIFI_NAN_SUPPORT 0
#include "esp_wifi_types.h"
#include "esp_wifi_he_types.h"
#undef CONFIG_SLAVE_SOC_WIFI_NAN_SUPPORT
}

}
