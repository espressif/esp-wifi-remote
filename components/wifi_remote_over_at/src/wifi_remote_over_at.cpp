/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <memory>
#include <cinttypes>
#include <cstring>
#include "esp_log.h"
#include "esp_wifi.h"
#include "cxx_include/esp_modem_dte.hpp"
#include "esp_modem_config.h"
#include "cxx_include/esp_modem_api.hpp"
#include "cxx_include/esp_modem_dce_factory.hpp"
#include "esp_netif.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "cxx_include/esp_modem_command_library_utils.hpp"

#if defined(CONFIG_ESP_WIFI_ENABLED)
extern "C" ESP_EVENT_DEFINE_BASE(WIFI_REMOTE_EVENT);
#else
#define WIFI_REMOTE_EVENT WIFI_EVENT
#endif

static const char *TAG = "wifi_rmt_over_at";

namespace at_rpc {

class ESP_AT_Module: public ::esp_modem::ModuleIf {
public:
    explicit ESP_AT_Module(std::shared_ptr<::esp_modem::DTE> dte, const esp_modem_dce_config *config):
            dte(std::move(dte)) {}

    bool setup_data_mode() override
    {
        return true;
    }

    bool set_mode(::esp_modem::modem_mode mode) override
    {
        ESP_LOGI(TAG, "Setting mode: %d", static_cast<int>(mode));
        if (mode == esp_modem::modem_mode::DATA_MODE) {
            return esp_modem::dce_commands::generic_command(dte.get(), "AT+PPPD\r\n", "CONNECT", "ERROR", 5000) == esp_modem::command_result::OK;
        }
        return true;
    }

protected:
    std::shared_ptr<::esp_modem::DTE> dte;
};

class DCE : public esp_modem::DCE_T<ESP_AT_Module> {
    using DCE_T<ESP_AT_Module>::DCE_T;
public:

    bool parse(std::string_view token, std::string_view pattern, std::string &out)
    {
        auto pos = token.find(pattern);
        if (pos != std::string::npos) {
            auto str_view = token.substr(++pos);
            // strip quotes if present
            auto quote1 = str_view.find('"');
            auto quote2 = str_view.rfind('"');
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                ESP_LOGV(TAG, "STR-view: {%.*s}\n", static_cast<int>(str_view.size()), str_view.data());
                out = str_view.substr(quote1 + 1, quote2 - quote1 - 1);
                ESP_LOGV(TAG, "IP_str: {%s}\n", out.c_str());
                return true;
            }
        }
        return false;
    }
    bool get_ip(std::string &ip, std::string &gw, std::string &mask)
    {
        ESP_LOGV(TAG, "%s", __func__);
        auto ret = dte->command("AT+CIPSTA?\r\n", [&](uint8_t *data, size_t len) {
            size_t pos = 0;
            std::string_view response((char *)data, len);
            while ((pos = response.find('\n')) != std::string::npos) {
                std::string_view token = response.substr(0, pos);
                for (auto it = token.end() - 1; it > token.begin(); it--) // strip trailing CR or LF
                    if (*it == '\r' || *it == '\n') {
                        token.remove_suffix(1);
                    }
                ESP_LOGV(TAG, "Token: {%.*s}\n", static_cast<int>(token.size()), token.data());
                parse(token, "CIPSTA:ip:", ip);
                parse(token, "CIPSTA:gateway:", gw);
                parse(token, "CIPSTA:netmask:", mask);
                if (token.find("OK") != std::string::npos) {
                    return esp_modem::command_result::OK;
                } else if (token.find("ERROR") != std::string::npos) {
                    return esp_modem::command_result::FAIL;
                }
                response = response.substr(pos + 1);
            }
            return esp_modem::command_result::TIMEOUT;
        }, 500);
        return ret == esp_modem::command_result::OK;
    }

    bool init()
    {
        for (int i=0; i<5; ++i) {
            if (sync() == esp_modem::command_result::OK) {
                ESP_LOGI(TAG, "Modem in sync");
                return true;
            }
            vTaskDelay(pdMS_TO_TICKS(500 * (i + 1)));
        }
        ESP_LOGE(TAG, "Failed to sync with esp-at");
        return false;
    }

    esp_modem::command_result sync()
    {
        auto ret = esp_modem::dce_commands::generic_command_common(dte.get(), "AT\r\n");
        ESP_LOGI(TAG, "Syncing with esp-at...(%d)", static_cast<int>(ret));
        return ret;
    }

    bool wifi_init(bool init)
    {
        auto command = "AT+CWINIT=" + std::string(init ? "1\r\n" : "0\r\n");
        return esp_modem::dce_commands::generic_command_common(dte.get(), command) == esp_modem::command_result::OK;
    }

    bool wifi_connect(std::string& ssid, std::string& password)
    {
        auto command = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"\r\n";
        auto ret =  esp_modem::dce_commands::generic_command_common(dte.get(), command, 5000);
        if (ret != esp_modem::command_result::OK) {
            ESP_LOGE(TAG, "Failed to connect to WiFi: %s", command.c_str());
            return false;
        }
        std::string ip, gw, mask;
        if (get_ip(ip, gw, mask)) {
            ESP_LOGI(TAG, "Connected to WiFi with IP: %s", ip.c_str());
            if (esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, on_ip, this) != ESP_OK) {
                printf("Failed to register IP event handler");
            }
            if (!set_mode(::esp_modem::modem_mode::DATA_MODE)) {
                ESP_LOGE(TAG, "Failed to set modem to DATA mode");
                return false;
            }
        }
        return true;
    }

    bool wifi_setmode(int mode)
    {
        auto command = "AT+CWMODE=" + std::to_string(mode) +  "\r\n"; //
        return esp_modem::dce_commands::generic_command_common(dte.get(), command) == esp_modem::command_result::OK;
    }

    bool wifi_disconnect()
    {
        auto command = "AT+CWQAP\r\n";
        return esp_modem::dce_commands::generic_command_common(dte.get(), command) == esp_modem::command_result::OK;
    }

private:
    static void on_ip(void *arg, esp_event_base_t base, int32_t event_id, void *data)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        esp_netif_t *netif = event->esp_netif;
        if (event_id == IP_EVENT_PPP_GOT_IP) {
            printf("Got IPv4 event: Interface \"%s(%s)\" address: " IPSTR, esp_netif_get_desc(netif),
                   esp_netif_get_ifkey(netif), IP2STR(&event->ip_info.ip));
            esp_event_post(IP_EVENT, IP_EVENT_STA_GOT_IP, event, sizeof(*event), 0);
            esp_netif_dns_info_t dns;
            dns.ip.u_addr.ip4.addr = esp_netif_htonl(0x08080808);
            dns.ip.type = ESP_IPADDR_TYPE_V4;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
        } else if (event_id == IP_EVENT_PPP_LOST_IP) {
            ESP_LOGI(TAG, "Disconnected");
        }
    }
};


class Factory: public ::esp_modem::dce_factory::Factory {
public:
    static std::unique_ptr<DCE> create(const esp_modem::dce_config *config, std::shared_ptr<esp_modem::DTE> dte, esp_netif_t *netif)
    {
        return build_generic_DCE<ESP_AT_Module, DCE, std::unique_ptr<DCE>>(config, std::move(dte), netif);
    }
};

std::unique_ptr<DCE> create(std::shared_ptr<esp_modem::DTE> dte)
{
    static esp_netif_t *netif;
    /* Configure the PPP netif */
    esp_netif_config_t netif_ppp_config = ESP_NETIF_DEFAULT_PPP();
    netif = esp_netif_new(&netif_ppp_config);
    assert(netif);

    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG("APN"); // dummy config (not used with esp-at)
    return Factory::create(&dce_config, std::move(dte), netif);
}

/**
 * @brief Engine that implements a simple RPC mechanism
 */
class RpcEngine {
public:
    constexpr explicit RpcEngine() : is_init_(false) {}

#if defined(CONFIG_WIFI_RMT_AT_FLOW_CONTROL_NONE)
#define FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_NONE
#elif defined(CONFIG_WIFI_RMT_AT_FLOW_CONTROL_SW)
#define FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_SW
#elif defined(CONFIG_WIFI_RMT_AT_FLOW_CONTROL_HW)
#define FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_HW
#endif

    esp_err_t init()
    {
        if (is_init_) {
            return ESP_OK;
        }
        esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
        /* setup UART specific configuration based on kconfig options */
        dte_config.uart_config.tx_io_num = CONFIG_WIFI_RMT_AT_UART_TX_PIN;
        dte_config.uart_config.rx_io_num = CONFIG_WIFI_RMT_AT_UART_RX_PIN;
        dte_config.uart_config.rts_io_num = CONFIG_WIFI_RMT_AT_UART_RTS_PIN;
        dte_config.uart_config.cts_io_num = CONFIG_WIFI_RMT_AT_UART_CTS_PIN;
        dte_config.uart_config.flow_control = FLOW_CONTROL;
        dte_config.uart_config.rx_buffer_size = CONFIG_WIFI_RMT_AT_UART_RX_BUFFER_SIZE;
        dte_config.uart_config.tx_buffer_size = CONFIG_WIFI_RMT_AT_UART_TX_BUFFER_SIZE;
        dte_config.uart_config.event_queue_size = CONFIG_WIFI_RMT_AT_UART_EVENT_QUEUE_SIZE;
        dte_config.task_stack_size = CONFIG_WIFI_RMT_AT_UART_EVENT_TASK_STACK_SIZE;
        dte_config.task_priority = CONFIG_WIFI_RMT_AT_UART_EVENT_TASK_PRIORITY;
        dte_config.dte_buffer_size = CONFIG_WIFI_RMT_AT_UART_RX_BUFFER_SIZE / 2;
        auto uart_dte = esp_modem::create_uart_dte(&dte_config);
        if (uart_dte == nullptr) {
            ESP_LOGE(TAG, "Failed to create UART DTE");
            return ESP_FAIL;
        }
        dce_ = create(std::move(uart_dte));
        if (!dce_->init()) {
            ESP_LOGE(TAG,  "Failed to setup network");
            return ESP_FAIL;
        }
        is_init_ = true;
        return ESP_OK;
    }

    esp_err_t wifi_init()
    {
        return dce_->wifi_init(true) ? ESP_OK : ESP_FAIL;
    }

    esp_err_t wifi_deinit()
    {
        return dce_->wifi_init(false) ? ESP_OK : ESP_FAIL;
    }
    static void s_task(void *task_param)
    {
        ESP_LOGI(TAG, "Starting connect task %p", task_param);
        auto t = static_cast<RpcEngine *>(task_param);
        t->connect();
        t->task_handle_ = nullptr;
        vTaskDelete(nullptr);
    }

    void connect()
    {
        ESP_LOGI(TAG, "Connecting...");
        auto ssid = std::string((char*)sta_config_.ssid);
        auto password = std::string((char*)sta_config_.password);
        dce_->wifi_connect(ssid, password);
        ESP_LOGI(TAG, "Connecting...done");
    }


    esp_err_t wifi_connect()
    {
        if (task_handle_) {
            return ESP_OK; // already connecting
        }
        return xTaskCreate(s_task, "connect_task", 4096, this, 5, &task_handle_) == pdPASS ? ESP_OK : ESP_FAIL;
    }

    esp_err_t wifi_disconnect()
    {
        return dce_->wifi_disconnect() ? ESP_OK : ESP_FAIL;
    }

    esp_err_t wifi_setmode(int mode)
    {
        return dce_->wifi_setmode(mode) ? ESP_OK : ESP_FAIL;
    }

    esp_err_t wifi_setconfig(wifi_interface_t interface, wifi_config_t *conf)
    {
        if (interface == WIFI_IF_AP) {
            ::memcpy(&ap_config_, &conf->ap, sizeof(ap_config_));
        } else if (interface == WIFI_IF_STA) {
            ::memcpy(&sta_config_, &conf->sta, sizeof(sta_config_));
        } else {
            ESP_LOGE(TAG, "Invalid interface %d", interface);
            return ESP_ERR_INVALID_ARG;
        }
        return ESP_OK;
    }

    void deinit()
    {
        if (!is_init_) {
            return;
        }
        is_init_ = false;
    }

private:
    bool is_init_;
    std::unique_ptr<DCE> dce_;
    wifi_ap_config_t  ap_config_{};
    wifi_sta_config_t sta_config_{};
    TaskHandle_t task_handle_{};

};

constinit RpcEngine instance;

}

extern "C" esp_err_t esp_wifi_remote_init(const wifi_init_config_t *config)
{

    if (at_rpc::instance.init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RPC engine");
        return ESP_FAIL;
    }
    return at_rpc::instance.wifi_init();
}

extern "C" esp_err_t esp_wifi_remote_set_config(wifi_interface_t interface, wifi_config_t *conf)
{
    return at_rpc::instance.wifi_setconfig(interface, conf);
}

extern "C" esp_err_t esp_wifi_remote_start(void)
{
    ESP_RETURN_ON_ERROR(esp_event_post(WIFI_REMOTE_EVENT, WIFI_EVENT_STA_START, nullptr, 0, 0), TAG, "Failed to start WiFi remote");
    return ESP_OK;
}

extern "C" esp_err_t esp_wifi_remote_stop(void)
{
    ESP_RETURN_ON_ERROR(esp_event_post(WIFI_REMOTE_EVENT, WIFI_EVENT_STA_STOP, nullptr, 0, 0), TAG, "Failed to stop WiFi remote");
    return ESP_OK;
}

extern "C" esp_err_t esp_wifi_remote_connect(void)
{
    return at_rpc::instance.wifi_connect();
}

extern "C" esp_err_t esp_wifi_remote_get_mac(wifi_interface_t ifx, uint8_t mac[6])
{
    // no-op, as we don't use slave's MAC in this implementation
    return ESP_OK;
}

extern "C" esp_err_t esp_wifi_remote_set_mode(wifi_mode_t mode)
{
    if (mode < WIFI_MODE_STA || mode > WIFI_MODE_APSTA) {
        ESP_LOGE(TAG, "Invalid WiFi mode: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }
    return at_rpc::instance.wifi_setmode(static_cast<int>(mode));
}

extern "C" esp_err_t esp_wifi_remote_deinit(void)
{
    return at_rpc::instance.wifi_deinit();
}

extern "C" esp_err_t esp_wifi_remote_disconnect(void)
{
    return at_rpc::instance.wifi_disconnect();
}

extern "C" esp_err_t esp_wifi_remote_set_storage(wifi_storage_t storage)
{
    return ESP_OK;
}
