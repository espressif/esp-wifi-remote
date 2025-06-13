# Sample application

This application is used to verify that WiFi remote over EPPP is buildable

## Test

```
readelf -s sample.elf | grep esp_wifi_remote_init
```

Checks that the function definitions linked into the project is from this library.
(Test added to CI)
