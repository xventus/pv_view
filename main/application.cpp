//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   application.h
/// @author Petr Vanek

#include "application.h"
#include "hardware.h"
#include "content_file.h"
#include "driver/uart.h"
#include "key_val.h"
#include <inttypes.h>

// global application instance as singleton and instance acquisition.

Application *Application::getInstance()
{
    static Application instance;
    return &instance;
}

Application::Application() : _taskEventGroup(nullptr)
{
}

Application::~Application()
{
    done();
    if (_taskEventGroup)
    {
        vEventGroupDelete(_taskEventGroup);
    }
}

void Application::init()
{
    // Begin initialization of core functions so that individual
    // wrappers no longer need to execute

    _taskEventGroup = xEventGroupCreate();
    if (!_taskEventGroup)
    {
        ESP_LOGE("Application", "Failed to create event group!");
        return;
    }

    // initi file system
    ConentFile::initFS();

    // initialize NVS
    KeyVal &kv = KeyVal::getInstance();
    kv.init(literals::kv_namespace, true, false);

    // initialize newtwork interfaces
    ESP_ERROR_CHECK(esp_netif_init());

    // default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void Application::run()
{
    // task create
    do
    {
        _connectionManager = std::make_shared<ConnectionManager>();

        if (!_connectionManager)
        {
            ESP_LOGE(TAG, "Failed to create ConnectionManager.");
        }

        if (!_wifiTask.init(_connectionManager, literals::tsk_web, tskIDLE_PRIORITY + 1ul, 4096))
            break;

        if (!_webTask.init(literals::tsk_wifi, tskIDLE_PRIORITY + 1ul, 2 * 4096))
            break;

        if (!_resetTask.init(literals::tsk_rst, tskIDLE_PRIORITY + 1ul, 4096))
            break;

        if (!_dsplTask.init(_connectionManager, literals::tsk_dspl, tskIDLE_PRIORITY + 1ul, 4096))
            break;

        if (!_mqttTask.init(_connectionManager, literals::tsk_mqtt, tskIDLE_PRIORITY + 1ul, 4096))
            break;

        if (!_timeTask.init(_connectionManager, literals::tsk_time, tskIDLE_PRIORITY + 1ul, 4096))
            break;


        waitForAllTasks({TaskBit::WiFi, TaskBit::Web, TaskBit::Reset, TaskBit::Display, TaskBit::Mqtt, TaskBit::Time});

        ESP_LOGI(TAG, "All tasks initialized. Proceeding...");

        // try to switch to wifi client mode
        getWifiTask()->switchMode(WifiTask::Mode::Client);

    } while (false);
}

void Application::done()
{
    // fail
    KeyVal &kv = KeyVal::getInstance();
    kv.done();
}

Application *Application::operator->()
{
    return Application::getInstance();
}

Application *Application::operator->() const
{
    return Application::getInstance();
}

void Application::signalTaskStart(TaskBit task)
{
    if (_taskEventGroup)
    {
        xEventGroupSetBits(_taskEventGroup, static_cast<EventBits_t>(task));
        ESP_LOGI(TAG, "Task %" PRIu32 " signaled start", static_cast<uint32_t>(task));
    }
}

void Application::waitForAllTasks(const std::vector<TaskBit> &tasks)
{
    if (!_taskEventGroup)
    {
        ESP_LOGE(TAG, "Event group not initialized!");
        return;
    }

    EventBits_t allBits = 0;
    for (const auto &task : tasks)
    {
        allBits |= static_cast<EventBits_t>(task);
    }

    EventBits_t bits = xEventGroupWaitBits(_taskEventGroup, allBits, pdFALSE, pdTRUE, portMAX_DELAY);

    if ((bits & allBits) == allBits)
    {
        ESP_LOGI(TAG, "All tasks have started successfully!");
    }
    else
    {
        ESP_LOGE(TAG, "Some tasks failed to start!");
    }
}
