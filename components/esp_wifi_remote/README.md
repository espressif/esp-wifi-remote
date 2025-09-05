# esp_wifi_remote

[![Component Registry](https://components.espressif.com/components/espressif/esp_wifi_remote/badge.svg)](https://components.espressif.com/components/espressif/esp_wifi_remote)

The `esp_wifi_remote` component provides transparent WiFi connectivity for ESP chipsets through external hardware, maintaining full `esp_wifi` API compatibility.

### Use Cases

**Non-WiFi ESP Chips**: Enables WiFi functionality on ESP32-P4, ESP32-H2 and other WiFi-less chipsets by routing `esp_wifi` calls to external WiFi hardware.

**Additional WiFi Interface**: On WiFi-capable ESP32 chips, provides dual WiFi interfaces - local `esp_wifi` and remote `esp_wifi_remote` for applications requiring multiple wireless connections.

To employ this component, a slave device -- capable of WiFi connectivity -- must be connected to your target device in a specified manner, as defined by the transport layer of [`esp_hosted`](https://github.com/espressif/esp-hosted).

Functionally, `esp_wifi_remote` wraps the public API of `esp_wifi`, offering a set of function call namespaces prefixed with esp_wifi_remote. These calls are translated into Remote Procedure Calls (RPC) to another target device (referred to as the "slave" device), which then executes the appropriate `esp_wifi` APIs.

Notably, `esp_wifi_remote` heavily relies on a specific version of the `esp_wifi` component. Consequently, the majority of its headers, sources, and configuration files are pre-generated based on the actual version of `esp_wifi`.

It's important to highlight that `esp_wifi_remote` does not directly implement the RPC calls; rather, it relies on dependencies for this functionality.

## Terminology

- **Backend Solution**: Communication layer handling transport of WiFi commands, events, and data between host and slave devices (e.g., esp-hosted, eppp, AT-based implementations)
- **Host-side**: Device running your application code (e.g., ESP32-P4, ESP32-H2, or ESP32 with WiFi)
- **Slave-side**: WiFi-capable device providing actual WiFi hardware and functionality

## Backend Solutions

`esp_wifi_remote` supports multiple backend solutions for RPC communication:

- **`esp_hosted`** (recommended): Plain text channels for WiFi API calls/events and Ethernet frames for data. Best performance and maturity.
- **`wifi_remote_over_eppp`**: SSL/TLS encrypted connection with PPP link. Suitable when encryption is required.
- **`wifi_remote_over_at`**: AT commands via esp-modem. Limited functionality but uses standard protocols.

### Performance Comparison

| Backend Solution     | Max TCP Throughput | Use Case |
|---------------------|-------------------|----------|
| esp_hosted          | ~50Mbps           | High-performance applications |
| wifi_remote_over_eppp | ~20Mbps         | Encrypted communication |
| wifi_remote_over_at | ~2Mbps            | Standard AT protocol compatibility |

## WiFi Configuration

Configure remote WiFi identically to local WiFi via Kconfig. Options use `WIFI_RMT_` prefix instead of `ESP_WIFI_`:

```
CONFIG_ESP_WIFI_TX_BA_WIN → CONFIG_WIFI_RMT_TX_BA_WIN
CONFIG_ESP_WIFI_AMPDU_RX_ENABLED → CONFIG_WIFI_RMT_AMPDU_RX_ENABLED
```

> **Note**: Some configuration options are compile-time only. Manual slave-side configuration and rebuild required for consistency.

## Dependencies on `esp_wifi`

Public API needs to correspond exactly to the `esp_wifi` API. Some of the internal types depend on the actual wifi target, as well as some default configuration values. Therefore it's easier to maintain consistency between this component and the exact version of `esp_wifi` automatically in CI:

* We extract function prototypes from `esp_wifi.h` and use them to generate `esp_wifi_remote` function declarations.
* We process the local `esp_wifi_types_native.h` and replace `CONFIG_IDF_TARGET` to `CONFIG_SLAVE_IDF_TARGET` and `CONFIG_SOC_WIFI_...` to `CONFIG_SLAVE_....`
* Similarly we process `esp_wifi`'s Kconfig, so the dependencies are on the slave target and slave SOC capabilities.

Please check the [README.md](./scripts/README.md) for more details on the generation step and testing consistency.
