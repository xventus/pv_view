//
// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   connection_manager.h
/// @author Petr Vanek

#pragma once

#include <memory>
#include <iostream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_eth.h>
#include <esp_netif.h>
#include <mqtt_client.h>
#include <nvs_flash.h>

class ConnectionManager
{
public:
    ConnectionManager()
    {
        event_group = xEventGroupCreate();
        if (event_group == NULL)
        {
            ESP_LOGE(LOG_TAG, "Failed to create event group");
        }
        else
        {
            // Clear all bits to ensure a known initial state
            xEventGroupClearBits(event_group, WIFI_CONNECTED_BIT | MQTT_CONNECTED_BIT);
        }
    }

    ~ConnectionManager()
    {
        if (event_group)
        {
            vEventGroupDelete(event_group);
        }
    }

    // Sets Wifi as connected
    void setConnected()
    {
        if (event_group)
        { // Check if event_group is not NULL
            xEventGroupSetBits(event_group, WIFI_CONNECTED_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setConnected");
        }
    }

    // Sets Wifi as disconnected
    void setDisconnected()
    {
        if (event_group)
        {
            xEventGroupClearBits(event_group, WIFI_CONNECTED_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setDisconnected");
        }
    }

    // Checks if Wifi is connected
    bool isConnected() const
    {
        if (event_group)
        {
            EventBits_t bits = xEventGroupGetBits(event_group);
            return bits & WIFI_CONNECTED_BIT;
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in isConnected");
            return false; // Assume disconnected if event group is NULL
        }
    }

    // Sets MQTT as active (connected)
    void setMqttActive()
    {
        if (event_group)
        {
            xEventGroupSetBits(event_group, MQTT_CONNECTED_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setMqttActive");
        }
    }

    // Sets MQTT as inactive (disconnected)
    void setMqttDeactive()
    {
        if (event_group)
        {
            xEventGroupClearBits(event_group, MQTT_CONNECTED_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setMqttDeactive");
        }
    }

    // Checks if MQTT is active
    bool isMqttActive() const
    {
        if (event_group)
        {
            EventBits_t bits = xEventGroupGetBits(event_group);
            return bits & MQTT_CONNECTED_BIT;
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in isMqttActive");
            return false; // Assume inactive if event group is NULL
        }
    }

    // Sets AP as active (connected)
    void setAPActive()
    {
        if (event_group)
        {
            xEventGroupSetBits(event_group, AP_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setAPActive");
        }
    }

    // Sets AP as inactive (disconnected)
    void setAPDeactive()
    {
        if (event_group)
        {
            xEventGroupClearBits(event_group, AP_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setAPDeactive");
        }
    }

    // Checks if AP is active
    bool isAPActive() const
    {
        if (event_group)
        {
            EventBits_t bits = xEventGroupGetBits(event_group);
            return bits & AP_BIT;
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in isAPActive");
            return false; // Assume inactive if event group is NULL
        }
    }

    // Sets TIME as active (synced)
    void setTimeActive()
    {
        if (event_group)
        {
            xEventGroupSetBits(event_group, TIME_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setTimeActive");
        }
    }

    // Sets AP as inactive (not synced)
    void setTimeDeactive()
    {
        if (event_group)
        {
            xEventGroupClearBits(event_group, TIME_BIT);
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in setTimeDeactive");
        }
    }

    // Checks if TIME is synced
    bool isTimeActive() const
    {
        if (event_group)
        {
            EventBits_t bits = xEventGroupGetBits(event_group);
            return bits & TIME_BIT;
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in isTimeActive");
            return false; // Assume inactive if event group is NULL
        }
    }

    // Returns the event group handle
    EventGroupHandle_t getEventGroup() const
    {
        if (event_group)
        {
            return event_group;
        }
        else
        {
            ESP_LOGE(LOG_TAG, "Event group is NULL in getEventGroup");
            return nullptr;
        }
    }

private:
    static constexpr const char *LOG_TAG = "ConnectionManager";

    EventGroupHandle_t event_group;
    static const int WIFI_CONNECTED_BIT = BIT0;
    static const int MQTT_CONNECTED_BIT = BIT1;
    static const int AP_BIT = BIT2;
    static const int TIME_BIT = BIT3;
};
