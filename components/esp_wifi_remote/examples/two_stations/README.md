# Simple example demonstrating Wi-Fi and Wi-Fi remote on the same chip

## How to use this example

* Run `server` example as the slave project (communication coprocessor)
* Select the backend solution
  - (in case of EPPP configure both the server and the client with mutual keys/certs as described in [mqtt example](../mqtt/README.md))
* Configure SSID and Password for two access points
  - one for local Wi-Fi (using the Wi-Fi library on the chip itself)
  - the second one for remote Wi-Fi (using the network offloading -- communication coprocessor)
* Build and run the example

## Wi-Fi types limitation

The `esp_wifi_remote` component faces type name conflicts when used alongside local Wi-Fi functionality. Both APIs use identical type names (e.g., `wifi_config_t`) but with different definitions for different hardware targets.

### The Solution: Separate Compilation Units

The component requires **separate compilation units** for local and remote Wi-Fi code:

```c
// two_stations.c - Local Wi-Fi only
#include "esp_wifi.h"

// remote_station.c - Remote Wi-Fi only
#include "injected/esp_wifi.h"  // SLAVE_ prefixed types
#include "esp_wifi_remote.h"
```

### How It Works

Wi-Fi remote provides generated "injected" headers with slave-specific types by transforming SOC capabilities:

```python
# generate_and_check.py transforms:
SOC_WIFI_ → SLAVE_SOC_WIFI_
IDF_TARGET_ → SLAVE_IDF_TARGET_
```

This allows the same Wi-Fi types to coexist with different definitions based on the target hardware.

### Best Practices

- Include injected headers before `esp_wifi_remote.h` in remote Wi-Fi files
- Keep local and remote Wi-Fi code in separate `.c` files
- Use function declarations to call between compilation units


## Example output

```
I (591) wifi:enable tsf
I (591) two wifi stations: Wi-Fi local events: WIFI_EVENT (43)
I (601) two wifi stations: Wi-Fi local events: WIFI_EVENT (2)
I (601) two wifi stations: wifi_init_sta finished.
I (611) wifi:new:<11,0>, old:<1,0>, ap:<255,255>, sta:<11,0>, prof:1, snd_ch_cfg:0x0
I (611) wifi:state: init -> auth (0xb0)
I (621) two wifi stations: Wi-Fi local events: WIFI_EVENT (43)
I (621) wifi:state: auth -> assoc (0x0)
I (641) wifi:state: assoc -> run (0x10)
I (671) wifi:security: WPA2-PSK, phy: bgn, rssi: -62
I (681) wifi:pm start, type: 1
I (681) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (681) wifi:set rx beacon pti, rx_bcn_pti: 0, bcn_timeout: 25000, mt_pti: 0, mt_time: 10000
I (691) two wifi stations: Wi-Fi local events: WIFI_EVENT (4)
I (1061) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (2691) esp_netif_handlers: sta ip: 192.168.32.221, mask: 255.255.254.0, gw: 192.168.32.3
I (2691) two wifi stations: Wi-Fi local events: IP_EVENT (0)
I (2691) two wifi stations: got ip:192.168.32.221
I (2701) two wifi stations: connected to ap SSID:EspressifSystems
I (2701) uart: queue free spaces: 16
I (2711) eppp_link: Waiting for IP address 0
I (2941) esp-netif_lwip-ppp: Connected
I (2951) eppp_link: Got IPv4 event: Interface "example_netif_sta(EPPP0)" address: 192.168.11.2
I (2951) esp-netif_lwip-ppp: Connected
I (2951) eppp_link: Connected! 0
I (5151) two wifi stations: wifi_init_remote_sta finished.
I (5211) two wifi stations: Wi-Fi remote events: WIFI_REMOTE_EVENT (43)
I (5211) two wifi stations: Wi-Fi remote events: WIFI_REMOTE_EVENT (2)
I (5461) two wifi stations: Wi-Fi remote events: WIFI_REMOTE_EVENT (4)
I (7341) esp_netif_handlers: sta ip: 192.168.11.2, mask: 255.255.255.255, gw: 192.168.11.1
I (7341) two wifi stations: Wi-Fi local events: IP_EVENT (0)
I (7341) two wifi stations: got ip:192.168.11.2
I (7351) two wifi stations: Wi-Fi remote events: IP_EVENT (0)
I (7361) two wifi stations: got ip:192.168.11.2
I (7361) rpc_client: Main DNS:192.168.32.3
I (7371) rpc_client: EPPP IP:192.168.11.1
I (7371) rpc_client: WIFI IP:192.168.33.131
I (7381) rpc_client: WIFI GW:192.168.32.3
I (7381) rpc_client: WIFI mask:255.255.254.0
I (7381) two wifi stations: connected to ap SSID:EspressifSystems
I (7391) main_task: Returned from app_main()
```
