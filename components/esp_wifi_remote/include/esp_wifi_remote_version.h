/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #pragma once

 #ifdef __cplusplus
 extern "C" {
 #endif

 /** Major version number (X.x.x) */
 #define ESP_WIFI_REMOTE_VERSION_MAJOR   1
 /** Minor version number (x.X.x) */
 #define ESP_WIFI_REMOTE_VERSION_MINOR   2
 /** Patch version number (x.x.X) */
 #define ESP_WIFI_REMOTE_VERSION_PATCH   5

 /**
  * Macro to convert IDF version number into an integer
  *
  * To be used in comparisons, such as ESP_WIFI_REMOTE_VERSION >= ESP_WIFI_REMOTE_VERSION_VAL(4, 0, 0)
  */
 #define ESP_WIFI_REMOTE_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))

 /**
  * Current IDF version, as an integer
  *
  * To be used in comparisons, such as ESP_WIFI_REMOTE_VERSION >= ESP_WIFI_REMOTE_VERSION_VAL(4, 0, 0)
  */
 #define ESP_WIFI_REMOTE_VERSION  ESP_WIFI_REMOTE_VERSION_VAL(ESP_WIFI_REMOTE_VERSION_MAJOR, \
                                              ESP_WIFI_REMOTE_VERSION_MINOR, \
                                              ESP_WIFI_REMOTE_VERSION_PATCH)

 #ifdef __cplusplus
 }
 #endif
