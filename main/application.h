//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   application.h
/// @author Petr Vanek

#pragma once

#include "freertos/FreeRTOS.h"
#include <memory>
#include <atomic>
#include <stdio.h>

#include "web_task.h"
#include "wifi_task.h"
#include "literals.h"
#include "reset_task.h"
#include "dspl_task.h"
#include "mqtt_task.h"
#include "time_task.h"
#include "connection_manager.h"
#include "freertos/event_groups.h"

/**
 * @brief application class - singleton
 *
 */
class Application
{

public:
    
    enum class TaskBit : uint32_t {
    WiFi = (1 << 0),  // Bit 0  WiFiTask
    Web = (1 << 1),   // Bit 1  WebTask
    Reset = (1 << 2), // Bit 2  ResetTask
    Display = (1 << 3), // Bit 3 DsplTask
    Mqtt = (1 << 4), // Bit 4 MqttTask
    Time = (1 << 5) // Bit 5 TimeTask
};
   
    /**
     * @brief Destroy the Application object
     *
     */
    virtual ~Application();

    /**
     * @brief application initialization
     *
     */
    void init();

    void run();

    /**
     * @brief application deinitialization, an error occurred
     *
     */
    void done();

    void waitForAllTasks(const std::vector<TaskBit> &tasks);

    void signalTaskStart(TaskBit task);

    WebTask *getWebTask() { return &_webTask;}
    WifiTask *getWifiTask() { return &_wifiTask;}
    ResetTask *getResetTask() {return  &_resetTask; }
    DisplayTask *getDisplayTask() {return  &_dsplTask; }
    MqttTask *getMqttTask() {return  &_mqttTask; }
    
    /**
     * Singleton
    */
    Application *operator->();
    Application* operator->() const;
    static Application *getInstance();

private:
   
    /**
     * @brief Constructor
     *
     */
    Application();
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    
    std::shared_ptr<ConnectionManager> _connectionManager;
    WebTask     _webTask;          ///< web interface
    WifiTask    _wifiTask;         ///< wifi AP / client  
    ResetTask   _resetTask;        ///< reset
    DisplayTask _dsplTask;         ///< display task
    MqttTask    _mqttTask;         ///< mqtt task
    TimeTask    _timeTask;         ///< time sync task
    EventGroupHandle_t _taskEventGroup{nullptr};


    static constexpr const char *TAG = "APP";
};
