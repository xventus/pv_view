//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   shoelace.h  Shoelace formula
/// @author Petr Vanek

#include <array>
#include <time.h>
#include <stdio.h>
#include "esp_log.h"





class Shoelace
{
private:
    static constexpr const char *TAG = "Shoelace";
    std::array<float, 24> hourlyConsumption = {0}; // Consumption in each hour (Wh)
    float lastPower = 0;                           // Last power value (W)
    time_t lastUpdateTime = 0;                     // Time of last update

public:
    
    using ChartUpdateCallback = std::function<void(int hour, float consumption)>;

    Shoelace() 
    {
        resetDailyConsumption();
    }

    void update(float currentPower) 
    {
        time_t now = time(NULL); 
        struct tm currentTime;
        localtime_r(&now, &currentTime); 

        if (lastUpdateTime != 0)
        {
            float timeDeltaHours = difftime(now, lastUpdateTime) / 3600.0; // Time difference in hours
            float energy = (lastPower + currentPower) / 2 * timeDeltaHours; // Shoelace formula

            // Add consumption to the current hour
            int currentHour = currentTime.tm_hour;
            hourlyConsumption[currentHour] += energy;
        }

        // We update the last performance and time
        lastPower = currentPower;
        lastUpdateTime = now;
    }

    void resetDailyConsumption()
    {
        hourlyConsumption.fill(0); 
    }

    void dump()
    {
        ESP_LOGI(TAG,"Hourly Consumption (Wh):");
        for (int hour = 0; hour < 24; hour++)
        {
            ESP_LOGI(TAG,"Hour %02d: %.2f Wh", hour, hourlyConsumption[hour]);
        }
    }

    float getConsumptionForHour(int hour)
    {
        if (hour >= 0 && hour < 24)
        {
            return hourlyConsumption[hour];
        }
        return 0;
    }

    void updateChart(const ChartUpdateCallback &callback)
    {
        for (int hour = 0; hour < 24; ++hour)
        {
            callback(hour, hourlyConsumption[hour]);
        }
    }
};
