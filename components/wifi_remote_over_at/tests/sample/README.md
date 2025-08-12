# Sample application

This application is used to verify that WiFi remote over AT is buildable

## Test

```
readelf -s sample.elf | grep esp_wifi_remote_init
```

Checks that the remote wifi function definitions were taken from `wifi_remote_over_at` component
