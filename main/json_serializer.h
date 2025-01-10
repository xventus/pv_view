//
// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   json_serializer.h
/// @author Petr Vanek

#pragma once

#include <string>
#include <string_view>
#include "cJSON.h"
#include "mqtt_queue_data.h"

class JsonSerializer
{
public:
    static void updateSolaxParameter(SolaxParameters &params, const std::string &key, int32_t value)
    {
        if (key == "PvVoltage1")
        {
            params.PvVoltage1 = value;
        }
        else if (key == "PvVoltage2")
        {
            params.PvVoltage2 = value;
        }
        else if (key == "PvCurrent1")
        {
            params.PvCurrent1 = value;
        }
        else if (key == "PvCurrent2")
        {
            params.PvCurrent2 = value;
        }
        else if (key == "Powerdc1")
        {
            params.Powerdc1 = value;
        }
        else if (key == "Powerdc2")
        {
            params.Powerdc2 = value;
        }
        else if (key == "BatVoltage_Charge1")
        {
            params.BatVoltage_Charge1 = value;
        }
        else if (key == "BatCurrent_Charge1")
        {
            params.BatCurrent_Charge1 = value;
        }
        else if (key == "Batpower_Charge1")
        {
            params.Batpower_Charge1 = value;
        }
        else if (key == "TemperatureBat")
        {
            params.TemperatureBat = value;
        }
        else if (key == "BattCap")
        {
            params.BattCap = value;
        }
        else if (key == "FeedinPower")
        {
            params.FeedinPower = value;
        }
        else if (key == "GridPower_R")
        {
            params.GridPower_R = value;
        }
        else if (key == "GridPower_S")
        {
            params.GridPower_S = value;
        }
        else if (key == "GridPower_T")
        {
            params.GridPower_T = value;
        }
        else if (key == "Etoday_togrid")
        {
            params.Etoday_togrid = value;
        }
        else if (key == "Temperature")
        {
            params.Temperature = value;
        }
        else if (key == "RunMode")
        {
            params.RunMode = value;
        }
        else if (key == "BDCStatus")
        {
            params.BDCStatus = value;
        }
        else if (key == "GridStatus")
        {
            params.GridStatus = value;
        }
        else if (key == "MPPTCount")
        {
            params.MPPTCount = value;
        }
        else if (key == "HDO")
        {
            params.Hdo = value;
        }
        else
        {
            ESP_LOGW("SolaxParameters", "Unknown key: %s", key.c_str());
        }
    }
    

    static void updateParametersFromJson(SolaxParameters &params, std::string_view jsonMessage) {
    if (jsonMessage.empty()) {
        ESP_LOGE("JSON", "EMPTY jsonMessage");
        return;
    }
    cJSON *json = cJSON_Parse(jsonMessage.data());
    if (!json) {
        ESP_LOGE("JSON", "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return;
    }

    cJSON *value = cJSON_GetObjectItem(json, "value");
    cJSON *name = cJSON_GetObjectItem(json, "name");

    if (cJSON_IsNumber(value) && cJSON_IsString(name)) {
        std::string nameStr = name->valuestring;
        int32_t valueInt = static_cast<int32_t>(value->valuedouble); 

        updateSolaxParameter(params, nameStr, valueInt);
    } else {
        ESP_LOGW("JSON", "Invalid JSON structure");
    }

    cJSON_Delete(json);
}


};

   