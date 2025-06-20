set(sources src/wifi_remote_rpc_client.cpp src/wifi_remote_rpc_server.cpp src/eppp_init.c)

idf_component_register(SRCS ${sources}
                       PRIV_INCLUDE_DIRS src
                       PRIV_REQUIRES esp_event esp_netif eppp_link esp-tls esp_wifi vfs)

# This will define the wifi_remote dependency to the wifi_remote_over_eppp
# also issues an error if wifi_remote is not found
idf_build_get_property(build_components BUILD_COMPONENTS)
if(esp_wifi_remote IN_LIST build_components)
    idf_component_get_property(wifi_remote esp_wifi_remote COMPONENT_LIB)
elseif(espressif__esp_wifi_remote IN_LIST build_components)
    idf_component_get_property(wifi_remote espressif__esp_wifi_remote COMPONENT_LIB)
else()
    message(FATAL_ERROR "This component requires esp_wifi_remote")
endif()
target_link_libraries(${wifi_remote} PRIVATE ${COMPONENT_LIB})
