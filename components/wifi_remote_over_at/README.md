# AT based implementation of Wi-Fi remote APIs

:warning: This component should be only used with `esp_wifi_remote`

This component has no public API, it only provides definitions of `esp_wifi_remote` functions.

It uses `esp_modem` as a host implementation, and `esp-at` at the communication coprocessor side.

## Build the host side

`esp_wifi_remote` does not provide a backend selection for `wifi_remote_over_at` directly. You need to choose "custom implementation" and add this component to the project dependencies.
- set  `CONFIG_ESP_WIFI_REMOTE_LIBRARY_CUSTOM=y`
- run `idf.py add-dependency wifi_remote_over_at`

## Build the slave side

The slave side uses `modem_sim` component from https://github.com/espressif/esp-protocols repository.
To build it, download:
- ESP-IDF v5.4@8ad0d3d8
- esp-protocols mqtt_cxx-v0.5.0

### Configure ESP-AT
- `cd common_components/modem_sim`
- `./install.sh [platform] [module] [tx-pin] [rx-pin]`
    - for example `./install.sh PLATFORM_ESP32C6 ESP32C6-4MB 22 23` for esp32c6 on the P4-C6 board
- `source export.sh`

### Build, flash, monitor

- `idf.py build flash monitor`
