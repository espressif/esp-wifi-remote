# Sample application

This application is used to verify that WiFi remote over EPPP is buildable

## Test (TODO)

```
readelf -s sample.elf | grep esp_wifi_remote_init
```

Checks that the strong function definitions was linked (instead of the weak symbol provided esp_wifi_remote)
