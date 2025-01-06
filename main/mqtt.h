//
// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   mqtt.h
/// @author Petr Vanek

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <functional>
#include <esp_log.h>
#include <mqtt_client.h>
#include <string>
#include <string_view>
#include <map>

using MqttConnectedCallback = std::function<void()>;
using MqttDisconnectedCallback = std::function<void()>;
using MqttMessageCallback = std::function<void(std::string_view topic, std::string_view message)>;

class Mqtt
{
public:
    static constexpr const char *LOG_TAG = "Mqtt";

    Mqtt()
        : _isConnected(false),
          _connectionMutex(xSemaphoreCreateMutex())
    {
        if (!_connectionMutex)
        {
            ESP_LOGE(LOG_TAG, "Failed to create mutex");
        }
    }

    ~Mqtt()
    {
        if (_client)
        {
            esp_mqtt_client_stop(_client);
            esp_mqtt_client_destroy(_client);
            _client = nullptr;
        }

        if (_connectionMutex)
        {
            vSemaphoreDelete(_connectionMutex);
            _connectionMutex = nullptr;
        }
    }

    bool init(std::string_view uri, std::string_view username = "", std::string_view password = "")
    {
        if (uri.empty())
        {
            ESP_LOGE(LOG_TAG, "Empty URI");
            return false;
        }

        if (!_connectionMutex)
        {
            ESP_LOGE(LOG_TAG, "Mutex is not initialized, cannot proceed");
            return false;
        }

        if (xSemaphoreTake(_connectionMutex, portMAX_DELAY) == pdTRUE)
        {
            if (_client)
            {
                ESP_LOGE(LOG_TAG, "MQTT is already initialized");
                xSemaphoreGive(_connectionMutex);
                return false;
            }

            esp_mqtt_client_config_t mqtt_cfg = {};
            mqtt_cfg.broker.address.uri = uri.data();
            mqtt_cfg.credentials.username = username.empty() ? nullptr : username.data();
            mqtt_cfg.credentials.authentication.password = password.empty() ? nullptr : password.data();

            _client = esp_mqtt_client_init(&mqtt_cfg);
            if (!_client)
            {
                ESP_LOGE(LOG_TAG, "Failed to initialize MQTT client");
                xSemaphoreGive(_connectionMutex);
                return false;
            }

            esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, &Mqtt::mqttEventHandler, this);
            esp_err_t ret = esp_mqtt_client_start(_client);
            _isConnected = (ret == ESP_OK);

            ESP_LOGI(LOG_TAG, "MQTT %s", _isConnected ? "started successfully" : "failed to start");
            xSemaphoreGive(_connectionMutex);

            return _isConnected;
        }
        return false;
    }

    void registerConnectedCallback(MqttConnectedCallback callback)
    {
        _connectedCallback = callback;
    }

    void registerDisconnectedCallback(MqttDisconnectedCallback callback)
    {
        _disconnectedCallback = callback;
    }

    bool isConnected() const
    {
        bool connected = false;
        if (xSemaphoreTake(_connectionMutex, portMAX_DELAY) == pdTRUE)
        {
            connected = _isConnected;
            xSemaphoreGive(_connectionMutex);
        }
        return connected;
    }

    bool publish(std::string_view topic, std::string_view data, int qos = 1, int retain = 0)
    {
        if (topic.empty() || data.empty())
        {
            ESP_LOGW(LOG_TAG, "topic / data is empty");
            return false;
        }

        if (!isConnected())
        {
            ESP_LOGW(LOG_TAG, "Cannot publish, MQTT is not connected");
            return false;
        }

        int msg_id = esp_mqtt_client_publish(_client, topic.data(), data.data(), 0, qos, retain);
        if (msg_id == -1)
        {
            ESP_LOGE(LOG_TAG, "Failed to publish message");
            return false;
        }

        ESP_LOGI(LOG_TAG, "Published message with ID %d on topic %s", msg_id, topic.data());
        return true;
    }

    bool subscribe(const std::string &topic, MqttMessageCallback callback)
    {
        if (topic.empty())
        {
            ESP_LOGE(LOG_TAG, "Topic cannot be empty");
            return false;
        }

        if (xSemaphoreTake(_connectionMutex, portMAX_DELAY) == pdTRUE)
        {
            if (!_isConnected)
            {
                ESP_LOGE(LOG_TAG, "MQTT is not connected");
                xSemaphoreGive(_connectionMutex);
                return false;
            }

            int msg_id = esp_mqtt_client_subscribe(_client, topic.c_str(), 1);
            if (msg_id == -1)
            {
                ESP_LOGE(LOG_TAG, "Failed to subscribe to topic: %s", topic.c_str());
                xSemaphoreGive(_connectionMutex);
                return false;
            }

            _topicCallbacks[topic] = callback; // Register the callback
            ESP_LOGI(LOG_TAG, "Subscribed to topic: %s", topic.c_str());
            xSemaphoreGive(_connectionMutex);
            return true;
        }
        return false;
    }

    bool unsubscribe(const std::string &topic)
    {
        if (topic.empty())
        {
            ESP_LOGE(LOG_TAG, "Topic cannot be empty");
            return false;
        }

        if (xSemaphoreTake(_connectionMutex, portMAX_DELAY) == pdTRUE)
        {
            // Check if the topic is registered
            auto it = _topicCallbacks.find(topic);
            if (it == _topicCallbacks.end())
            {
                ESP_LOGW(LOG_TAG, "Topic %s is not registered", topic.c_str());
                xSemaphoreGive(_connectionMutex);
                return false;
            }

            // Unsubscribe from the topic
            int msg_id = esp_mqtt_client_unsubscribe(_client, topic.c_str());
            if (msg_id == -1)
            {
                ESP_LOGE(LOG_TAG, "Failed to unsubscribe from topic: %s", topic.c_str());
                xSemaphoreGive(_connectionMutex);
                return false;
            }

            // Remove the topic from the callback map
            _topicCallbacks.erase(it);

            ESP_LOGI(LOG_TAG, "Unsubscribed from topic: %s", topic.c_str());
            xSemaphoreGive(_connectionMutex);
            return true;
        }
        return false;
    }

private:
    static void mqttEventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
    {

        esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
        Mqtt *client = static_cast<Mqtt *>(handler_args);
        switch (event_id)
        {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(LOG_TAG, "MQTT Connected");
            if (xSemaphoreTake(client->_connectionMutex, portMAX_DELAY) == pdTRUE)
            {
                client->_isConnected = true;
                xSemaphoreGive(client->_connectionMutex);
            }
            if (client->_connectedCallback)
            {
                client->_connectedCallback();
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(LOG_TAG, "MQTT Disconnected");
            if (xSemaphoreTake(client->_connectionMutex, portMAX_DELAY) == pdTRUE)
            {
                client->_isConnected = false;
                xSemaphoreGive(client->_connectionMutex);
            }
            if (client->_disconnectedCallback)
            {
                client->_disconnectedCallback();
            }
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(LOG_TAG, "Message published successfully");
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(LOG_TAG, "MQTT Error occurred");
            break;

        case MQTT_EVENT_DATA:
        {
            std::string topic(event->topic, event->topic_len);
            std::string message(event->data, event->data_len);

            // ESP_LOGI(LOG_TAG, "Received data on topic: %s", topic.c_str());

            auto it = client->_topicCallbacks.find(topic);
            if (it != client->_topicCallbacks.end() && it->second)
            {
                it->second(topic, message); // Invoke the callback
            }
            break;
        }

        default:
            break;
        }
    }

    bool _isConnected;
    esp_mqtt_client_handle_t _client{nullptr};
    SemaphoreHandle_t _connectionMutex;
    MqttConnectedCallback _connectedCallback{};
    MqttDisconnectedCallback _disconnectedCallback{};
    std::map<std::string, MqttMessageCallback> _topicCallbacks;
};
