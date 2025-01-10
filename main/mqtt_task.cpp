//
// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   mqtt_task.cpp
/// @author Petr Vanek

#include <cstdint>
#include <atomic>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "cJSON.h"
#include "mqtt_task.h"
#include "literals.h"
#include "application.h"
#include "json_serializer.h"
#include "key_val.h"

MqttTask::MqttTask() : _mqttClient(nullptr), _mqttInitialized(false)
{
}

bool MqttTask::init(std::shared_ptr<ConnectionManager> connMgr,
                    const char *name, UBaseType_t priority, const configSTACK_DEPTH_TYPE stackDepth)
{
    bool rc = false;
    _connectionManager = connMgr;
    rc = RPTask::init(name, priority, stackDepth);
    return rc;
}

MqttTask::~MqttTask()
{
    done();
}

void MqttTask::initializeMqttClient()
{

    KeyVal &kv = KeyVal::getInstance();
    // Only create the client if it hasn't been created
    if (!_mqttClient)
    {
        _mqttClient = std::make_unique<Mqtt>();
        // Set up MQTT callbacks
        _mqttClient->registerConnectedCallback([&]()
                                               {
                                                   ESP_LOGI(LOG_TAG, "Connected to MQTT broker");
                                                   if (_connectionManager)
                                                       _connectionManager->setMqttActive();
                                                   Application::getInstance()->getDisplayTask()->settingMsg("Connected to MQTT broker"); });
        _mqttClient->registerDisconnectedCallback([&]()
                                                  {
            ESP_LOGI(LOG_TAG, "Disconnected from MQTT broker");
             if (_connectionManager) _connectionManager->setMqttDeactive(); 
             Application::getInstance()->getDisplayTask()->settingMsg("Disconnected from MQTT broker"); });

        std::string mqtt;
        mqtt = "mqtt://";
        mqtt += kv.readString(literals::kv_mqtt);
        if (!_mqttClient->init(mqtt, kv.readString(literals::kv_user), kv.readString(literals::kv_passwd)))
        {
            ESP_LOGE(LOG_TAG, "Failed to initialize MQTT client");
            Application::getInstance()->getDisplayTask()->settingMsg("Failed to initialize MQTT client");
        }
    }
}

void MqttTask::doneMqttClient()
{
    // Only create the client if it hasn't been created
    if (_mqttClient)
    {
        _mqttClient.reset();
    }
}

void MqttTask::loop()
{
    KeyVal &kv = KeyVal::getInstance();
    auto topic = kv.readString(literals::kv_topic, "solax/data");
    Application::getInstance()->signalTaskStart(Application::TaskBit::Mqtt);
    bool subscribe = false;
    uint16_t counter = 0;  
    std::memset(&_solaxData, 0, sizeof(SolaxParameters));
    Application::getInstance()->getDisplayTask()->updateUI(_solaxData);
    while (true)
    { // Loop forever

        // Check connection status
        if (_connectionManager && _connectionManager->isConnected())
        {
            if (!_mqttInitialized)
            {
                initializeMqttClient(); // Initialize MQTT only once upon connection
                _mqttInitialized = true;
            }
        }
        else
        {
            // Connection lost
            if (_mqttInitialized)
            {
                // Clean up the MQTT client if it was previously initialized
                doneMqttClient();
            }
            _mqttInitialized = false;
        }

        if (_connectionManager && _connectionManager->isMqttActive() && !subscribe)
        {
            ESP_LOGI(LOG_TAG, "Topic registration [%s]", topic.c_str());
            subscribe = true;
            _mqttClient->subscribe(topic, [this, &counter](std::string_view topic, std::string_view message) { 
                JsonSerializer::updateParametersFromJson(_solaxData, message);
                counter++;

                // number of necessary data received for GUI update
                if (counter > 20) {
                    counter = 0; 
                    Application::getInstance()->getDisplayTask()->updateUI(_solaxData);
                }
                });
        }

        if (_connectionManager && !_connectionManager->isMqttActive() && subscribe)
        {
            //ESP_LOGI(LOG_TAG, "Topic remove [%s]", topic.c_str());
            //_mqttClient->unsubscribe(topic);
            //subscribe = false;
        }


        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
