if(NOT CONFIG_ESP_WIFI_ENABLED AND NOT CONFIG_ESP_HOST_WIFI_ENABLED)
    set(wifi_remote_sources esp_wifi_remote.c esp_wifi_remote_net.c)
else()
    set(wifi_remote_sources esp_wifi_remote_net2.c)
endif()

idf_component_register(INCLUDE_DIRS include
                    SRCS ${wifi_remote_sources}
                    REQUIRES esp_event esp_netif
                    PRIV_REQUIRES esp_wifi esp-tls vfs)

set(IDF_VER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/idf_v${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")

# Check if we're on tagged version of ESP-IDF and if we need to supply specific version
string(REGEX MATCH "^v[0-9]+\\.[0-9]+(\\.[0-9]+)?(-dirty)?$" IDF_VER_TAG "${IDF_VER}")
if (IDF_VER_TAG)
    string(REGEX REPLACE "-dirty$" "" IDF_VER_TAG "${IDF_VER_TAG}")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/idf_tag_${IDF_VER_TAG}")
        set(IDF_VER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/idf_tag_${IDF_VER_TAG}")
    endif()
endif()

idf_component_get_property(wifi esp_wifi COMPONENT_LIB)

if(NOT CONFIG_ESP_WIFI_ENABLED AND NOT CONFIG_ESP_HOST_WIFI_ENABLED)
    set(src_wifi_with_remote ${IDF_VER_DIR}/esp_wifi_with_remote.c)
    if(CONFIG_ESP_WIFI_REMOTE_EAP_ENABLED)
        list(APPEND src_wifi_with_remote ${IDF_VER_DIR}/esp_eap_client_with_remote.c)
    endif()
    # We need to build wifi sources with wifi-remote properties, so the injected wifi headers are used
    get_target_property(wifi_sources ${wifi} SOURCES)
    set_target_properties(${wifi} PROPERTIES SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/dummy_src.c")
endif()

# Add weak symbol for the wifi_remote API if we're using esp_hosted implementation
if(CONFIG_ESP_WIFI_REMOTE_LIBRARY_HOSTED)
    set(src_wifi_remote_weak ${IDF_VER_DIR}/esp_wifi_remote_weak.c)
    if(CONFIG_ESP_WIFI_REMOTE_EAP_ENABLED)
        list(APPEND src_wifi_remote_weak ${IDF_VER_DIR}/esp_eap_client_remote_weak.c)
    endif()
endif()

target_include_directories(${COMPONENT_LIB} PUBLIC   ${IDF_VER_DIR}/include)
target_sources(${COMPONENT_LIB}             PRIVATE  ${src_wifi_remote_weak}
                                                     ${src_wifi_with_remote}
                                                     ${wifi_sources})

if(NOT CONFIG_ESP_WIFI_ENABLED AND NOT CONFIG_ESP_HOST_WIFI_ENABLED)
    # Update wifi include directories to prepend the injected dir with modified headers supporting SLAVE capability
    get_target_property(original_wifi_dirs ${wifi} INTERFACE_INCLUDE_DIRECTORIES)
    set(updated_wifi_dirs "${IDF_VER_DIR}/include/injected" ${original_wifi_dirs})
    set_target_properties(${wifi} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${updated_wifi_dirs}")
endif()

target_link_libraries(${wifi} PUBLIC ${COMPONENT_LIB})


if(CONFIG_ESP_WIFI_REMOTE_EAP_ENABLED)
    idf_component_optional_requires(PRIVATE wpa_supplicant)
endif()

if(CONFIG_ESP_WIFI_REMOTE_LIBRARY_EPPP)
    idf_component_optional_requires(PRIVATE wifi_remote_over_eppp espressif__wifi_remote_over_eppp)
endif()
