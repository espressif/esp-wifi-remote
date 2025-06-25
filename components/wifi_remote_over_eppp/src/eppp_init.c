/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_log.h"
#include "esp_wifi.h"
#include "eppp_link.h"

__attribute__((weak)) esp_netif_t *wifi_remote_eppp_init(eppp_type_t role)
{
    uint32_t our_ip = role == EPPP_SERVER ? EPPP_DEFAULT_SERVER_IP() : EPPP_DEFAULT_CLIENT_IP();
    uint32_t their_ip = role == EPPP_SERVER ? EPPP_DEFAULT_CLIENT_IP() : EPPP_DEFAULT_SERVER_IP();
    eppp_config_t config = EPPP_DEFAULT_CONFIG(our_ip, their_ip);
    // We currently support only UART and SPI transport
#ifdef CONFIG_EPPP_LINK_DEVICE_UART
    config.transport = EPPP_TRANSPORT_UART;
    config.uart.tx_io = CONFIG_WIFI_RMT_OVER_EPPP_UART_TX_PIN;
    config.uart.rx_io = CONFIG_WIFI_RMT_OVER_EPPP_UART_RX_PIN;
    config.uart.port = CONFIG_WIFI_RMT_OVER_EPPP_UART_PORT;
    config.ppp.netif_description = CONFIG_WIFI_RMT_OVER_EPPP_NETIF_DESCRIPTION;
    config.ppp.netif_prio = CONFIG_WIFI_RMT_OVER_EPPP_NETIF_PRIORITY;
    return eppp_open(role, &config, portMAX_DELAY);
#elif CONFIG_EPPP_LINK_DEVICE_SPI
    config.transport = EPPP_TRANSPORT_SPI;
    config.spi.host = CONFIG_WIFI_RMT_OVER_EPPP_SPI_HOST;
    config.spi.mosi = CONFIG_WIFI_RMT_OVER_EPPP_SPI_MOSI_PIN;
    config.spi.miso = CONFIG_WIFI_RMT_OVER_EPPP_SPI_MISO_PIN;
    config.spi.sclk = CONFIG_WIFI_RMT_OVER_EPPP_SPI_SCLK_PIN;
    config.spi.cs = CONFIG_WIFI_RMT_OVER_EPPP_SPI_CS_PIN;
    config.spi.intr = CONFIG_WIFI_RMT_OVER_EPPP_SPI_INTR_PIN;
    config.spi.freq = CONFIG_WIFI_RMT_OVER_EPPP_SPI_FREQ;
    config.ppp.netif_description = CONFIG_WIFI_RMT_OVER_EPPP_NETIF_DESCRIPTION;
    config.ppp.netif_prio = CONFIG_WIFI_RMT_OVER_EPPP_NETIF_PRIORITY;
    return eppp_open(role, &config, portMAX_DELAY);
#else
    return ESP_ERR_INVALID_STATE;
#endif
}
