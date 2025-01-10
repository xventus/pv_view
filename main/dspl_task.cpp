
//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   led_task.cpp
/// @author Petr Vanek

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "dspl_task.h"
#include "application.h"
#include "esp_log.h"
#include "key_val.h"
#include "literals.h"
#include "utils.h"

DisplayTask::DisplayTask() : _consumption("cons"), _photovoltaic("pv"), _sdcard("/sdcard", HW_SD_MOSI, HW_SD_MISO, HW_SD_CLK, HW_SD_CS)
{
    _queue = xQueueCreate(5, sizeof(DisplayTask::ReqData));
    _queueData = xQueueCreate(5, sizeof(SolaxParameters));

    // display adriver init adn attach to lvgl
    _dd.initBus();
    _dd.start();
}

DisplayTask::~DisplayTask()
{
    done();
    if (_queue)
        vQueueDelete(_queue);

    if (_queueData)
        vQueueDelete(_queueData);
}

void DisplayTask::loop()
{
    bool mountOK = false;
    bool loadAfterReset = true;
    int lastMin = 0;
    bool firstCheck = true;

    mountOK = (_sdcard.mount(true) == ESP_OK);
    if (!mountOK)
        ESP_LOGE(TAG, "SD card mount failed - memory mode");
    else
        ESP_LOGW(TAG, "SD card mode active");

    // display adjust & show initial screen
    _dd.rotation(LV_DISP_ROT_NONE);
    _dd.lock();
    _dashboard.createScreen(nullptr);
    _dashboard.createSettingsScreen(); // for diagnostic messages
    _dashboard.clearAllDataSets();

    // calback for creating AP
    _dashboard.registerApSettingCallback([this]()
                                         {
                                             ESP_LOGI(TAG, "AP setting started...");
                                             _dashboard.disableSettingsApButton();
                                             Application::getInstance()->getWifiTask()->switchMode(WifiTask::Mode::AP); });

    _dd.unlock();
    _dd.setBrightness(100);
    bool lastMqtt = false;
    bool lastConnection = _connectionManager ? _connectionManager->isConnected() : false;
    Application::getInstance()->signalTaskStart(Application::TaskBit::Display);

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ReqData req;
        if (xQueueReceive(_queue, &req, 0) == pdTRUE)
        {
            if (req.contnet == Contnet::UpdateData)
            {
                // UI error message
                _dd.lock();
                _dashboard.updateSettingsTextArea(static_cast<const char *>(req.msg));
                _dd.unlock();
            }
        }
        if (xQueueReceive(_queueData, &_SolaxData, 0) == pdTRUE)
        {
            // update UI data
            _dd.lock();
            _dashboard.hdoUpdate(_SolaxData.Hdo);
            _dashboard.updateSolarPanels(_SolaxData.Powerdc1, _SolaxData.Powerdc2);
            _dashboard.updateBattery(_SolaxData.BattCap, _SolaxData.Batpower_Charge1, _SolaxData.TemperatureBat);
            auto inverterTotal = _SolaxData.GridPower_R + _SolaxData.GridPower_S + _SolaxData.GridPower_T;
            auto consumption = inverterTotal - _SolaxData.FeedinPower;
            auto photovoltaic = _SolaxData.Powerdc1 + _SolaxData.Powerdc2;
            auto freeEnergy = photovoltaic - consumption;

            _dashboard.updateConsumption(consumption);
            _dashboard.updateGrid(_SolaxData.FeedinPower, (_SolaxData.GridStatus == 0));
            _dashboard.updateOverview(inverterTotal, _SolaxData.Temperature);
            _dashboard.updateEnergyBar(freeEnergy);

            auto [dt, tm] = Utils::getDateTime();
            ESP_LOGI(TAG, "Utils::getDateTime %s %s", dt.c_str(), tm.c_str());
            _dashboard.updateDateTime(dt, tm);
            // ---> valid time
            if (_connectionManager && _connectionManager->isTimeActive())
            {
                auto [hour, min, sec] = Utils::getTime();

                if (hour == 0 && min == 0 && lastMin != min)
                { // Day reset,
                    lastMin = min;
                    _consumption.resetDailyConsumption();
                    _photovoltaic.resetDailyConsumption();
                    auto filename = "/" + Utils::getDayFileName();
                    _sdcard.deleteFile(filename);
                    filename += ".pv";
                    _sdcard.deleteFile(filename);
                    _dashboard.clearAllDataSets();
                }
                else
                {

                    if (mountOK && loadAfterReset)
                    {
                        loadAfterReset = false;
                        auto filename = "/" + Utils::getDayFileName();
                        _consumption.load(_sdcard.readFile(filename));
                        filename += ".pv";
                        _photovoltaic.load(_sdcard.readFile(filename));

                        _consumption.updateChart([this](int hour, float consumption)
                                                 { _dashboard.updateDataSetHour(1, hour, consumption); });
                        _photovoltaic.updateChart([this](int hour, float consumption)
                                                  { _dashboard.updateDataSetHour(0, hour, consumption); });
                    }

                    _consumption.update(consumption);
                    _photovoltaic.update(photovoltaic);

                    //ESP_LOGI(TAG, "Total %ld  Sol %ld", (int32_t)_photovoltaic.getSum(), (int32_t)_consumption.getSum());
                    _dashboard.updateTotal((_SolaxData.Etoday_togrid/10)*1000 /* _photovoltaic.getSum()*/, (int)_consumption.getSum());
                    _dashboard.updateDataSetHour(1, hour, _consumption.getConsumptionForHour(hour));
                    _dashboard.updateDataSetHour(0, hour, _photovoltaic.getConsumptionForHour(hour));

                    // if (sec == 0 || sec == 20 || sec == 40)
                    ESP_LOGI(TAG, "TIME %d %d %d", hour, min, sec);

                    if (mountOK && (min % 5 == 0) && (lastMin != min))
                    {
                        lastMin = min;
                        auto filename = "/" + Utils::getDayFileName();
                        if (_sdcard.writeFile(filename, _consumption.save()))
                        {
                            ESP_LOGI(TAG, "File written successfully %s", filename.c_str());
                        }
                        filename += ".pv";
                        if (_sdcard.writeFile(filename, _photovoltaic.save()))
                        {
                            ESP_LOGI(TAG, "File written successfully %s", filename.c_str());
                        }
                    }
                }

            } // <--- valid time

            _dd.unlock();
        }

        // chcek connection error
        if (_connectionManager)
        {
            bool currentConnection = _connectionManager->isConnected();
            if (lastConnection != currentConnection || firstCheck)
            {
                firstCheck = false;
                lastConnection = currentConnection;
                _dd.lock();
                ESP_LOGI(TAG, "Connection changed: Wifi %s", lastConnection ? "CONNECTED" : "DISCONNECTED");
                if (lastConnection)
                {
                    _dashboard.setEnergyBar();
                }
                else
                {
                    _dashboard.setNowWifi();
                }
                _dd.unlock();
            }

            bool currentMqtt = _connectionManager->isMqttActive();
            if (lastMqtt != currentMqtt)
            {
                lastMqtt = currentMqtt;
                _dd.lock();
                ESP_LOGI(TAG, "MQTT changed: %s", lastMqtt ? "ACTIVE" : "INACTIVE");
                if (!lastMqtt)
                {
                    _dashboard.setNoMqtt();
                }
                else
                {
                    _dashboard.setEnergyBar();
                }
                _dd.unlock();
            }
        }
    }

    _sdcard.unmount(); // never umnounted!!!
}

void DisplayTask::settingMsg(std::string_view msg)
{

    if (_queue)
    {
        ReqData rqdt;
        rqdt.contnet = Contnet::UpdateData;

        // Ensure the message fits within the buffer, leaving room for the null-terminator
        size_t maxLength = sizeof(rqdt.msg) - 1;
        size_t copyLength = std::min(msg.size(), maxLength);
        std::memcpy(rqdt.msg, msg.data(), copyLength);
        rqdt.msg[copyLength] = '\0';
        xQueueSendToBack(_queue, &rqdt, 0);
    }
}

void DisplayTask::updateUI(const SolaxParameters &msg)
{
    if (_queueData)
    {
        xQueueSendToBack(_queueData, &msg, 0);
    }
}

bool DisplayTask::init(std::shared_ptr<ConnectionManager> connMgr, const char *name, UBaseType_t priority, const configSTACK_DEPTH_TYPE stackDepth)
{
    bool rc = false;
    _connectionManager = connMgr;
    rc = RPTask::init(name, priority, stackDepth);
    return rc;
}