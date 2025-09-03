# Wi-Fi remote

Enables Espressif WiFi capabilities on remote targets (without native WiFi support).

This repository contains components and utilities related to esp-wifi-remote.

## Overview

esp-wifi-remote provides transparent WiFi connectivity for ESP chipsets through external hardware, maintaining full compatibility with the standard `esp_wifi` API. This enables WiFi functionality on ESP32-P4, ESP32-H2, and other WiFi-less chipsets by routing API calls to a connected WiFi-capable device.

The solution supports multiple backend communication protocols and can also provide additional WiFi interfaces on WiFi-capable ESP32 chips for applications requiring dual wireless connectivity.

## esp-wifi-remote

[![Component Registry](https://components.espressif.com/components/espressif/esp_wifi_remote/badge.svg)](https://components.espressif.com/components/espressif/esp_wifi_remote)

[esp_wifi_remote](https://github.com/espressif/esp-wifi-remote/tree/main/components/esp_wifi_remote/README.md)

## wifi-remote-over-eppp

[![Component Registry](https://components.espressif.com/components/espressif/wifi_remote_over_eppp/badge.svg)](https://components.espressif.com/components/espressif/wifi_remote_over_eppp)

[wifi_remote_over_eppp](https://github.com/espressif/esp-wifi-remote/tree/main/components/wifi_remote_over_eppp/README.md)

## wifi-remote-over-at

[![Component Registry](https://components.espressif.com/components/espressif/wifi_remote_over_at/badge.svg)](https://components.espressif.com/components/espressif/wifi_remote_over_at)

[wifi_remote_over_at](https://github.com/espressif/esp-wifi-remote/tree/main/components/wifi_remote_over_at/README.md)

## esp-hosted

The recommended backend solution for esp-wifi-remote is [esp-hosted](https://github.com/espressif/esp-hosted), providing optimal performance (up to 50Mbps TCP throughput), mature integration, and comprehensive support for ESP32-based host-slave communication.
