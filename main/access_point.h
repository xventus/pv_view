
/**
 * @file access_point.h
 * @author Petr Vanek (petr@fotoventus.cz)
 * @brief AP & config Server 
 * @version 0.1
 * @date 2022-04-05
 * 
 * @copyright Copyright (c) 2022 Petr Vanek
 * 
 */
#pragma once
#include <string>
#include <cstring>
#include <functional>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"


using WiFiApConnectedCallback = std::function<void(uint8_t* mac, uint8_t aid)>;
using WiFiApDisconnectedCallback = std::function<void(uint8_t* mac, uint8_t aid)>;

class WiFiAccessPoint {
public:
    WiFiAccessPoint()
        : _instanceAnyId(nullptr), _coreinit(false), _ap(nullptr) {}

    ~WiFiAccessPoint() {
        stop();
    }

    void start(const std::string& ssid, const std::string& password) {
        _ap = esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_config_t wifi_config = {};
        strncpy(reinterpret_cast<char*>(wifi_config.ap.ssid), ssid.c_str(), sizeof(wifi_config.ap.ssid));
        wifi_config.ap.ssid_len = ssid.length();
        strncpy(reinterpret_cast<char*>(wifi_config.ap.password), password.c_str(), sizeof(wifi_config.ap.password));
        wifi_config.ap.max_connection = 4;
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

        if (password.empty()) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                           ESP_EVENT_ANY_ID,
                                                           &WiFiAccessPoint::wifi_event_handler,
                                                           this,
                                                           &_instanceAnyId));
    }

    void stop() {
        if (_instanceAnyId) {
            esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _instanceAnyId);
            _instanceAnyId = nullptr;
        }

        esp_wifi_stop();
        esp_wifi_deinit();
        esp_wifi_set_mode(WIFI_MODE_NULL);
        if (_ap) {
            esp_netif_destroy(_ap);
            _ap = nullptr;
        }

        if (_coreinit) {
            ESP_ERROR_CHECK(esp_netif_deinit());
            nvs_flash_deinit();
        }

    }

    // Function to register client connected callback
    void registerClientConnectedCallback(WiFiApConnectedCallback& callback) {
        _clientConnectedCallback = callback;
    }

    // Function to register client disconnected callback
    void registerClientDisconnectedCallback(WiFiApDisconnectedCallback& callback) {
        _clientDisconnectedCallback = callback;
    }

    void onClientConnected(uint8_t* mac, uint8_t aid) {
        if (_clientConnectedCallback) {
            _clientConnectedCallback(mac, aid);
        } else {
            // Default behavior if no callback is set
            printf("Client connected: AID=%d\n", aid);
        }
    }

    void onClientDisconnected(uint8_t* mac, uint8_t aid) {
        if (_clientDisconnectedCallback) {
            _clientDisconnectedCallback(mac, aid);
        } else {
            // Default behavior if no callback is set
            printf("Client disconnected: AID=%d\n", aid);
        }
    }

private:
    static constexpr const char *TAG = "AP";
    esp_event_handler_instance_t _instanceAnyId;
    bool _coreinit;
    esp_netif_t* _ap;

    // Callbacks
    WiFiApConnectedCallback _clientConnectedCallback{nullptr};
    WiFiApDisconnectedCallback _clientDisconnectedCallback{nullptr};

    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
        WiFiAccessPoint* wifiAccessPoint = static_cast<WiFiAccessPoint*>(arg);
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
            auto* event = static_cast<wifi_event_ap_staconnected_t*>(event_data);
            wifiAccessPoint->onClientConnected(event->mac, event->aid);
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            auto* event = static_cast<wifi_event_ap_stadisconnected_t*>(event_data);
            wifiAccessPoint->onClientDisconnected(event->mac, event->aid);
        }
    }
};


