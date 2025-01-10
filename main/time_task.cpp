
//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   time_task.cpp
/// @author Petr Vanek

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "time_task.h"
#include "esp_log.h"
#include "key_val.h"
#include "literals.h"
#include "application.h"
#include "esp_sntp.h"
#include "esp_timer.h"

TimeTask::TimeTask()
{
}

TimeTask::~TimeTask()
{
  done();
}


void TimeTask::initializeSNTP()
{
  KeyVal &kv = KeyVal::getInstance();
	_timeserver = kv.readString(literals::kv_timeserver, literals::kv_def_timeserver);

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0,  _timeserver.c_str()); 
  sntp_init();
  ESP_LOGI(LOG_TAG, "SNTP initialized. Time synchronization started. [%s]", _timeserver.c_str());
}

void TimeTask::updateTimeZone(const char* tmz)
{ 
  if (tmz) {
    setenv("TZ", tmz, 1);
    tzset();
    ESP_LOGI(LOG_TAG, "Time zone set to CET with daylight saving. [%s]", tmz);
  } else {
    ESP_LOGE(LOG_TAG, "Time zone - null");
  }
}

bool  TimeTask::syncTime()
{
  auto rc = false;
  sntp_sync_status_t status = sntp_get_sync_status();
  if (status == SNTP_SYNC_STATUS_RESET)
  {
    ESP_LOGW(LOG_TAG, "Time not synchronized. Retrying...");
    Application::getInstance()->getDisplayTask()->settingMsg("Time not synchronized. Retrying...");
    sntp_restart();
  }
  else if (status == SNTP_SYNC_STATUS_IN_PROGRESS)
  {
    ESP_LOGI(LOG_TAG, "Time synchronization in progress...");
    Application::getInstance()->getDisplayTask()->settingMsg("Time synchronization in progress...");
  }
  else
  {
    time_t now = time(NULL);
    struct tm timeinfo;
    KeyVal &kv = KeyVal::getInstance();
    const auto zone = kv.readString(literals::kv_timezone, literals::kv_def_timezone);
    updateTimeZone(zone.c_str());
    localtime_r(&now, &timeinfo);
    ESP_LOGW(LOG_TAG, "Time synchronized: %s status=%d   zone [%s]", asctime(&timeinfo),status, zone.c_str());
    Application::getInstance()->getDisplayTask()->settingMsg(asctime(&timeinfo));
    if (_connectionManager)  _connectionManager->setTimeActive();
    rc = true;
  }

  return rc;
}

bool TimeTask::init(std::shared_ptr<ConnectionManager> connMgr,
                    const char *name, UBaseType_t priority, const configSTACK_DEPTH_TYPE stackDepth)
{
  bool rc = false;
  _connectionManager = connMgr;
  rc = RPTask::init(name, priority, stackDepth);
  return rc;
}

void TimeTask::loop()
{
    Application::getInstance()->signalTaskStart(Application::TaskBit::Time);

    if (_connectionManager)  _connectionManager->setTimeDeactive();

    int syncIntervalMs = 3600; // in ms
    int64_t lastSyncTime = esp_timer_get_time();
    bool initialSyncDone = false;

    while (true)
    {
        if (_connectionManager && _connectionManager->isConnected())
        {
            if (!initialSyncDone)
            {
                ESP_LOGI(LOG_TAG, "Wi-Fi connected. Initializing SNTP...");
                if (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) 
                {
                    initializeSNTP(); // Initialize SNTP only if it hasn't been started
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                initialSyncDone = true;
            }

            int64_t now = esp_timer_get_time();
            int64_t elapsedTime = (now - lastSyncTime) / 1000; // Convert to ms

            if (elapsedTime >= syncIntervalMs)
            {
                ESP_LOGI(LOG_TAG, "Syncing time...");
                if (!syncTime())
                {
                      // wait more time
                      syncIntervalMs = 10000;
                      ESP_LOGI(LOG_TAG, "Wait more time");
                } else {
                    syncIntervalMs = 1000000;                    
                }

                lastSyncTime = esp_timer_get_time();
            }
        }
        else
        {
            if (initialSyncDone)
            {
                ESP_LOGW(LOG_TAG, "Wi-Fi disconnected. Waiting for reconnection...");
                initialSyncDone = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}
