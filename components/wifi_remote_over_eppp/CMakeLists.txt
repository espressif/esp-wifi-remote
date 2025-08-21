if(NOT CONFIG_ESP_WIFI_REMOTE_LIBRARY_EPPP AND NOT CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register()
    return()
endif()

set(sources src/wifi_remote_rpc_client.cpp src/wifi_remote_rpc_server.cpp src/eppp_init.c)

idf_component_register(SRCS ${sources}
                       PRIV_INCLUDE_DIRS src
                       PRIV_REQUIRES esp_event esp_netif eppp_link esp-tls esp_wifi vfs)
