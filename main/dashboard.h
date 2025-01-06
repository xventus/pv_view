// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   dashboard.cpp
/// @author Petr Vanek
#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <iomanip>
#include <sstream>

#include "esp_log.h"
#include "display_driver.h"
#include "icons8_solar_panel_48.h"
#include "icons8_car_battery_48.h"
#include "icons8_telephone_pole_48.h"
#include "icons8_home_48.h"
#include "icons8_generator_32.h"
#include "icons8_error_30.h"
#include "icons8_wifi_24.h"

class Dashboard
{
private:
    static constexpr const char *TAG = "DD";

    std::function<void()> _apSettingCallback{nullptr};

    lv_obj_t *_settingsScreen{nullptr};   // Second screen - configuration
    lv_obj_t *_settingsExitBtn{nullptr};  // Exit button
    lv_obj_t *_settingsApBtn{nullptr};    // AP settings button
    lv_obj_t *_settingsTextArea{nullptr}; // Text area

    lv_obj_t *_screen{nullptr};
    lv_obj_t *_solarPanelFrame{nullptr};
    lv_obj_t *_batteryFrame{nullptr};
    lv_obj_t *_consumptionFrame{nullptr};
    lv_obj_t *_gridFrame{nullptr};
    lv_obj_t *_overviewFrame{nullptr};
    lv_obj_t *_energyBarFrame{nullptr};

    lv_obj_t *_gridLed{nullptr};
    lv_obj_t *_solarPanelTotalPowerLabel{nullptr};
    lv_obj_t *_solarPanelString1Label{nullptr};
    lv_obj_t *_solarPanelString2Label{nullptr};
    lv_obj_t *_batteryPercentageLabel{nullptr};
    lv_obj_t *_batteryPowerLabel{nullptr};
    lv_obj_t *_batteryTempLabel{nullptr};
    lv_obj_t *_consumptionLabel{nullptr};
    lv_obj_t *_gridLabel{nullptr};
    lv_obj_t *_overviewPowerLabel{nullptr};
    lv_obj_t *_overviewTempLabel{nullptr};
    lv_obj_t *_energyBarLabel{nullptr};
    lv_obj_t *_energyBar{nullptr};

    lv_obj_t *_chartTitleLabel{nullptr}; // For the chart title label
    lv_obj_t *_chart{nullptr};           // For the chart object

    lv_chart_series_t *_chartSeries{nullptr};
    int _currentDataSetIndex{0};
    int _lastDataSetIndex{0};

    enum EnergyBarMode
    {
        BAR,
        MESSAGE
    };

    EnergyBarMode _energyBarMode = BAR; // Default mode

    lv_obj_t *_messageIcon{nullptr};
    lv_obj_t *_messageLabel{nullptr};

    struct Message
    {
        const lv_img_dsc_t *icon;
        const char *text;
    };

    struct ChartDataSet
    {
        const char *description;
        lv_color_t color;
        int data[24];
    };

    ChartDataSet _dataSets[2] = {
        {"Photovoltaic Yield", lv_color_hex(0x007BFF), {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        {"Energy Consumption", lv_color_hex(0xFF4500), {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
        };

public:
    void disableSettingsApButton()
    {
        if (_settingsApBtn)
        {
            lv_obj_add_state(_settingsApBtn, LV_STATE_DISABLED);
            ESP_LOGI(TAG, "Settings AP button disabled.");
        }
    }

    void enableSettingsApButton()
    {
        if (_settingsApBtn)
        {
            lv_obj_clear_state(_settingsApBtn, LV_STATE_DISABLED);
            ESP_LOGI(TAG, "Settings AP button enabled.");
        }
    }

    void setEnergyBarMode(EnergyBarMode mode)
    {
        _energyBarMode = mode;
        if (_energyBarMode == BAR)
        {
            showEnergyBar();
        }
        else
        {
            showEnergyMessage();
        }
    }

    void setNowWifi()
    {
        const Message msg = {
            &icons8_wifi_24,
            "No connection"};
        updateEnergyMessage(msg);
        setEnergyBarMode(EnergyBarMode::MESSAGE);
    }

    void setNoMqtt()
    {
        const Message msg = {
            &icons8_error_30,
            "I can't connect to MQTT"};
        updateEnergyMessage(msg);
        setEnergyBarMode(EnergyBarMode::MESSAGE);
    }

    void setEnergyBar()
    {
        setEnergyBarMode(EnergyBarMode::BAR);
    }

    void updateEnergyMessage(const Message &message)
    {
        if (!_messageIcon)
        {
            _messageIcon = lv_img_create(_energyBarFrame);
            lv_obj_align(_messageIcon, LV_ALIGN_LEFT_MID, 10, 0);
        }
        if (!_messageLabel)
        {
            _messageLabel = lv_label_create(_energyBarFrame);
            lv_obj_set_style_text_font(_messageLabel, &lv_font_montserrat_14, 0);
            lv_obj_align(_messageLabel, LV_ALIGN_LEFT_MID, 50, 0);
        }

        lv_img_set_src(_messageIcon, message.icon);
        lv_label_set_text(_messageLabel, message.text);
    }

    void registerApSettingCallback(const std::function<void()> &callback)
    {
        _apSettingCallback = callback;
    }

    void createScreen(lv_obj_t *parent)
    {
        if (_screen && lv_obj_is_valid(_screen))
        {
            ESP_LOGE(TAG, "createScreen already exists");
            return;
        }

        _screen = lv_obj_create(parent);
        lv_scr_load(_screen);

        // Set screen background color
        lv_obj_set_style_bg_color(_screen, lv_color_hex(0x99AAFF), 0); // Light blue-gray
        lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);             // Full opacity

        createFrames();
    }

    void createSettingsScreen()
    {

        if (_settingsScreen && lv_obj_is_valid(_settingsScreen))
        {
            ESP_LOGE(TAG, "createSettingsScreen already exists");
            return;
        }
        _settingsScreen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(_settingsScreen, lv_color_hex(0xFFFFFF), 0); // White background
        lv_obj_set_style_bg_opa(_settingsScreen, LV_OPA_COVER, 0);

        // Exit button
        _settingsExitBtn = lv_btn_create(_settingsScreen);
        lv_obj_set_size(_settingsExitBtn, 100, 50);
        lv_obj_align(_settingsExitBtn, LV_ALIGN_TOP_LEFT, 20, 20);
        lv_obj_t *exitLabel = lv_label_create(_settingsExitBtn);
        lv_label_set_text(exitLabel, "EXIT");
        lv_obj_center(exitLabel);
        lv_obj_add_event_cb(_settingsExitBtn, [](lv_event_t *e)
                            {
            Dashboard *dashboard = static_cast<Dashboard *>(lv_event_get_user_data(e));
            if (dashboard) {
        dashboard->showMainScreen();
        } else {
            ESP_LOGE(TAG, "Dashboard is null!");
        } }, LV_EVENT_CLICKED, this);

        // Setting via AP button
        _settingsApBtn = lv_btn_create(_settingsScreen);
        lv_obj_set_size(_settingsApBtn, 150, 50);
        lv_obj_align(_settingsApBtn, LV_ALIGN_TOP_RIGHT, -20, 20);
        lv_obj_t *apLabel = lv_label_create(_settingsApBtn);
        lv_label_set_text(apLabel, "Setting via AP");
        lv_obj_center(apLabel);
        lv_obj_add_event_cb(_settingsApBtn, [](lv_event_t *e)
                            {
                                Dashboard *dashboard = static_cast<Dashboard *>(lv_event_get_user_data(e));
                                if (dashboard && dashboard->_apSettingCallback)
                                {
                                    dashboard->_apSettingCallback();
                                }

                                ESP_LOGI(TAG, "Setting via AP clicked!"); }, LV_EVENT_CLICKED, this);

        // Text area
        _settingsTextArea = lv_textarea_create(_settingsScreen);
        lv_obj_set_size(_settingsTextArea, 280, 200);
        lv_obj_align(_settingsTextArea, LV_ALIGN_BOTTOM_MID, 0, -200);
        lv_textarea_set_text(_settingsTextArea, "After connecting to the AP and entering http://192.168.4.1 in the browser, the configuration page is displayed.");
        lv_obj_set_style_text_color(_settingsTextArea, lv_color_hex(0x000000), 0); // Black text

        // Set the text area to read-only
        lv_obj_add_flag(_settingsTextArea, LV_OBJ_FLAG_CLICKABLE); // Disable interaction
    }

    void showMainScreen()
    {
        if (_screen && lv_obj_is_valid(_screen))
        {
            lv_scr_load(_screen);
            ESP_LOGI(TAG, "Main screen displayed.");
        }
        else
        {
            ESP_LOGE(TAG, "Main screen is invalid.");
        }
    }

    void showSettingsScreen()
    {
        if (!_settingsScreen)
        {
            createSettingsScreen();
        }
        lv_scr_load(_settingsScreen);
        ESP_LOGI(TAG, "Settings screen displayed.");
    }

    void enableOverviewClick()
    {
        lv_obj_add_event_cb(_overviewFrame, [](lv_event_t *e)
                            {
            Dashboard *dashboard = static_cast<Dashboard *>(lv_event_get_user_data(e));
            dashboard->showSettingsScreen(); }, LV_EVENT_CLICKED, this);
        ESP_LOGI(TAG, "Overview click event enabled.");
    }

    void createFrames()
    {
        // Solar Panels
        _solarPanelFrame = createFrame(152, 120, LV_ALIGN_TOP_LEFT, 5, 5, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _solarPanelTotalPowerLabel = addLabel(_solarPanelFrame, "1000 W", LV_ALIGN_TOP_MID, 0, -5, &lv_font_montserrat_14);
        addIcon(_solarPanelFrame, &icons8_solar_panel_48, LV_ALIGN_CENTER, 0, 0);
        _solarPanelString1Label = addLabel(_solarPanelFrame, "520 W", LV_ALIGN_CENTER, -30, 35, &lv_font_montserrat_12);
        _solarPanelString2Label = addLabel(_solarPanelFrame, "480 W", LV_ALIGN_CENTER, 30, 35, &lv_font_montserrat_12);

        // Battery
        _batteryFrame = createFrame(152, 120, LV_ALIGN_TOP_RIGHT, -5, 5, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _batteryPercentageLabel = addLabel(_batteryFrame, "80%", LV_ALIGN_TOP_MID, 0, -5, &lv_font_montserrat_14);
        addIcon(_batteryFrame, &icons8_car_battery_48, LV_ALIGN_CENTER, 0, 0);
        _batteryPowerLabel = addLabel(_batteryFrame, "500 W", LV_ALIGN_CENTER, -30, 35, &lv_font_montserrat_12);
        _batteryTempLabel = addTemperatureLabel(_batteryFrame, "35.0 °C", LV_ALIGN_CENTER, 30, 35);

        // Consumption
        _consumptionFrame = createFrame(85, 110, LV_ALIGN_TOP_RIGHT, -5, 130, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _consumptionLabel = addLabel(_consumptionFrame, "532 W", LV_ALIGN_TOP_MID, 0, -5, &lv_font_montserrat_14);
        addIcon(_consumptionFrame, &icons8_home_48, LV_ALIGN_CENTER, 0, 0);

        // grid
        _gridFrame = createFrame(85, 110, LV_ALIGN_TOP_LEFT, 5, 130, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _gridLabel = addLabel(_gridFrame, "781 W", LV_ALIGN_TOP_MID, 0, -5, &lv_font_montserrat_14);
        addIcon(_gridFrame, &icons8_telephone_pole_48, LV_ALIGN_CENTER, 0, 0);
        _gridLed = lv_led_create(_gridFrame);
        lv_obj_set_size(_gridLed, 5, 5); // Set LED size
        lv_obj_align(_gridLed, LV_ALIGN_BOTTOM_LEFT, 5, -5); 

        // Overview
        _overviewFrame = createFrame(130, 110, LV_ALIGN_TOP_MID, 0, 130, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _overviewPowerLabel = addLabel(_overviewFrame, "3500 W", LV_ALIGN_TOP_MID, 0, -5, &lv_font_montserrat_14);
        _overviewTempLabel = addTemperatureLabel(_overviewFrame, "50.0 °C", LV_ALIGN_BOTTOM_MID, 0, -5);
        addIcon(_overviewFrame, &icons8_generator_32, LV_ALIGN_CENTER, 0, 0);

        // Energy Bar
        _energyBarFrame = createFrame(310, 80, LV_ALIGN_CENTER, 0, 44, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _energyBarLabel = addLabel(_energyBarFrame, "1200 W", LV_ALIGN_TOP_MID, 0, -10, &lv_font_montserrat_14);
        _energyBar = addBar(_energyBarFrame, 270, 20, LV_ALIGN_BOTTOM_MID, 0, -10);

        createBarGraph("24-Hour Energy Usage", lv_color_hex(0x007BFF));

        enableOverviewClick();
    }

    lv_obj_t *createFrame(int width, int height, lv_align_t align, int x, int y, lv_color_t borderColor, lv_color_t bgColor)
    {
        lv_obj_t *frame = lv_obj_create(_screen);
        lv_obj_set_size(frame, width, height);
        lv_obj_align(frame, align, x, y);

        // Frame style
        lv_obj_set_style_radius(frame, 10, 0);
        lv_obj_set_style_border_width(frame, 4, 0);
        lv_obj_set_style_border_color(frame, borderColor, 0);
        lv_obj_set_style_bg_color(frame, bgColor, 0);
        lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);

        return frame;
    }

    lv_obj_t *addLabel(lv_obj_t *parent, const char *text, lv_align_t align, int x, int y, const lv_font_t *font)
    {
        lv_obj_t *label = lv_label_create(parent);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_font(label, font, 0);
        lv_obj_set_style_text_color(label, lv_color_black(), 0);
        lv_obj_align(label, align, x, y);
        return label;
    }

    lv_obj_t *addTemperatureLabel(lv_obj_t *parent, const char *text, lv_align_t align, int x, int y)
    {
        lv_obj_t *label = lv_label_create(parent);
        lv_label_set_text(label, text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        updateTemperatureTextColor(label, atof(text));
        lv_obj_align(label, align, x, y);
        return label;
    }

    void updateTemperatureTextColor(lv_obj_t *label, float temp)
    {
        if (temp < 15)
        {
            lv_obj_set_style_text_color(label, lv_color_hex(0x0000FF), 0); // Blue for cold
        }
        else if (temp <= 30)
        {
            lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0); // Black for normal
        }
        else
        {
            lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0); // Orange for hot
        }
    }

    void addIcon(lv_obj_t *parent, const lv_img_dsc_t *icon, lv_align_t align, int x, int y)
    {
        lv_obj_t *img = lv_img_create(parent);
        lv_img_set_src(img, icon);
        lv_obj_align(img, align, x, y);
    }

    lv_obj_t *addBar(lv_obj_t *parent, int width, int height, lv_align_t align, int x, int y)
    {
        lv_obj_t *bar = lv_bar_create(parent);
        lv_obj_set_size(bar, width, height);
        lv_obj_align(bar, align, x, y);
        lv_bar_set_range(bar, 0, 11000);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        return bar;
    }

    void updateSolarPanels(int string1Power, int string2Power)
{
    char text[20];
    int totalPower = string1Power + string2Power;

  
    sprintf(text, "%d W", totalPower);
    lv_label_set_text(_solarPanelTotalPowerLabel, text);

  
    sprintf(text, "%d W", string1Power);
    lv_label_set_text(_solarPanelString1Label, text);

    sprintf(text, "%d W", string2Power);
    lv_label_set_text(_solarPanelString2Label, text);

    
    if (totalPower > 100)
    {
       
        lv_obj_set_style_border_color(_solarPanelFrame, lv_color_hex(0x00FF00), 0); 
    }
    else
    {
       
        lv_obj_set_style_border_color(_solarPanelFrame, lv_color_hex(0x000000), 0);
    }
}

    void updateBattery(int percentage, int power, float temp)
    {
        char text[20];
        sprintf(text, "%d%%", percentage);
        lv_label_set_text(_batteryPercentageLabel, text);

        sprintf(text, "%d W", power);
        lv_label_set_text(_batteryPowerLabel, text);

        sprintf(text, "%.1f °C", temp);
        lv_label_set_text(_batteryTempLabel, text);
        updateTemperatureTextColor(_batteryTempLabel, temp);

        // Change the frame color based on power
        if (power > 100)
        {
            // Dark green for positive power
            lv_obj_set_style_border_color(_batteryFrame, lv_color_hex(0x00EE00), 0);
        }
        else if (power < 100)
        {
            // Dark red for negative power
            lv_obj_set_style_border_color(_batteryFrame, lv_color_hex(0xEE0000), 0);
        }
        else
        {
            // Black for zero power
            lv_obj_set_style_border_color(_batteryFrame, lv_color_hex(0x000000), 0);
        }
    }

    void updateConsumption(int power)
    {
        updateLabel(_consumptionLabel, power);
    }

    std::string formatPower(int powr)
    {
    std::ostringstream oss;
    if (powr < 1000)
    {
        oss << powr << " W";
    }
    else
    {
        double kilowatts = powr / 1000.0; // Převod na kilowatty
        oss << std::fixed << std::setprecision(2) << kilowatts << " kW";
    }
    return oss.str();
    }

    void updateGrid(int power, bool ongrid)
    {
        updateLabel(_gridLabel, power);

        if (ongrid)
        {
            lv_led_set_color(_gridLed, lv_color_hex(0x00FF00)); // Green for online
            lv_led_on(_gridLed); // Turn the LED on
        }
        else
        {
            lv_led_set_color(_gridLed, lv_color_hex(0xFF0000)); // Red for offline
            lv_led_on(_gridLed); // Turn the LED on
        }

        // Change the frame color based on power
        if (power > 100)
        {
            // Dark green for power greater than +500 W
            lv_obj_set_style_border_color(_gridFrame, lv_color_hex(0x00EE00), 0);
        }
        else if (power < -100)
        {
            // Dark red for power less than -500 W
            lv_obj_set_style_border_color(_gridFrame, lv_color_hex(0xEE0000), 0);
        }
        else
        {
            // Black for power in range -500 to +500 W
            lv_obj_set_style_border_color(_gridFrame, lv_color_hex(0x000000), 0);
        }
    }

    void updateOverview(int power, float temp)
    {
        updateLabel(_overviewPowerLabel, power);
        char tempText[20];
        sprintf(tempText, "%.1f °C", temp);
        lv_label_set_text(_overviewTempLabel, tempText);
        updateTemperatureTextColor(_overviewTempLabel, temp);
    }

    void updateEnergyBar(int value)
    {
        if (value < 0)
        {
            lv_obj_set_style_bg_color(_energyBar, lv_color_hex(0xFF0000), LV_PART_INDICATOR); // Red for negative
        }
        else
        {
            lv_obj_set_style_bg_color(_energyBar, lv_color_hex(0x00FF00), LV_PART_INDICATOR); // Green for positive
        }
        lv_bar_set_value(_energyBar, abs(value), LV_ANIM_ON);
        updateLabel(_energyBarLabel, value);
    }

    void createBarGraph(const char *title, lv_color_t barColor)
    {
        // Create a frame for the bar graph
        lv_obj_t *barGraphFrame = createFrame(310, 140, LV_ALIGN_BOTTOM_MID, 0, -10, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));

        // Add a title label to the frame
        _chartTitleLabel = addLabel(barGraphFrame, title, LV_ALIGN_TOP_MID, 0, -10, &lv_font_montserrat_14);

        // Create the bar graph
        _chart = lv_chart_create(barGraphFrame);
        lv_obj_set_size(_chart, 260, 80); // Set the size of the bar graph
        lv_obj_align(_chart, LV_ALIGN_CENTER, 0, 10);

        // Configure the chart as a bar graph
        lv_chart_set_type(_chart, LV_CHART_TYPE_LINE);
        lv_chart_set_point_count(_chart, 24); // 24 hours of data

        // Set the Y-axis range
        lv_chart_set_range(_chart, LV_CHART_AXIS_PRIMARY_Y,10, 10000); // Assuming max value is 12000W

        // Customize the appearance
        lv_obj_set_style_bg_color(_chart, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_chart, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_pad_gap(_chart, 0, LV_PART_MAIN); // Set the gap between bars
        lv_obj_set_style_border_width(_chart, 0, LV_PART_MAIN);

        // Configure horizontal grid lines every 1000W
        lv_chart_set_div_line_count(_chart, 10, 0); // 10  divisions for 1000W steps (0 to 12000W)

        // Remove vertical grid lines
        lv_obj_set_style_pad_column(_chart, 0, LV_PART_MAIN);

        // Create the series and assign it to _chartSeries
        _chartSeries = lv_chart_add_series(_chart, barColor, LV_CHART_AXIS_PRIMARY_Y);

        // Populate the series with initial data
        for (int i = 0; i < 24; i++)
        {
            lv_chart_set_value_by_id(_chart, _chartSeries, i, _dataSets[0].data[i]);
        }

        // Make the chart clickable
        lv_obj_add_flag(_chart, LV_OBJ_FLAG_CLICKABLE);

        // Add an event listener for the chart
        lv_obj_add_event_cb(_chart, [](lv_event_t *e)
                            {
        Dashboard *dashboard = static_cast<Dashboard *>(lv_event_get_user_data(e));
        ESP_LOGI(TAG,"Chart clicked");
        dashboard->onChartClick(); }, LV_EVENT_CLICKED, this);
    }

    // Event handler for chart title click
    void onChartClick()
    {
        ESP_LOGI(TAG, "onChartClick called");

        // Ensure chart and series are valid
        if (!_chart)
        {
            ESP_LOGE(TAG, "Error: _chart is null");
            return;
        }

        if (!_chartSeries)
        {
            ESP_LOGE(TAG, "Error: _chartSeries is null");
            return;
        }

        // Increment the current dataset index
        _currentDataSetIndex = (_currentDataSetIndex + 1) % 2;

        // Update chart title
        lv_label_set_text(_chartTitleLabel, _dataSets[_currentDataSetIndex].description);

        // Update series color
        ESP_LOGI(TAG, "Setting series color to: 0x%X\n", _dataSets[_currentDataSetIndex].color.full);
        lv_chart_set_series_color(_chart, _chartSeries, _dataSets[_currentDataSetIndex].color);

        // Update chart data
        for (int i = 0; i < 24; i++)
        {
            lv_chart_set_value_by_id(_chart, _chartSeries, i, _dataSets[_currentDataSetIndex].data[i]);
        }

        ESP_LOGI(TAG, "Dataset switched to: %s\n", _dataSets[_currentDataSetIndex].description);
    }

    void clearAllDataSets()
    {
        for (int datasetIndex = 0; datasetIndex < 3; datasetIndex++)
        {
            for (int hour = 0; hour < 24; hour++)
            {
                _dataSets[datasetIndex].data[hour] = 0;
            }
        }
        ESP_LOGI(TAG, "All data sets cleared\n");

        // Optionally update the chart if it's active
        if (_chartSeries && _chart)
        {
            for (int i = 0; i < 24; i++)
            {
                lv_chart_set_value_by_id(_chart, _chartSeries, i, 0);
            }
        }
    }

    void updateDataSet(int datasetIndex, const int newData[24])
    {
        if (datasetIndex < 0 || datasetIndex >= 3)
        {
            ESP_LOGI(TAG, "Invalid dataset index: %d\n", datasetIndex);
            return;
        }

        for (int i = 0; i < 24; i++)
        {
            _dataSets[datasetIndex].data[i] = newData[i];
        }
        ESP_LOGI(TAG, "Dataset %d updated\n", datasetIndex);

        // If the updated dataset is currently displayed, refresh the chart
        if (datasetIndex == _currentDataSetIndex && _chartSeries && _chart)
        {
            for (int i = 0; i < 24; i++)
            {
                lv_chart_set_value_by_id(_chart, _chartSeries, i, newData[i]);
            }
        }
    }

    void updateDataSetHour(int datasetIndex, int hour, int newValue)
    {
        if (datasetIndex < 0 || datasetIndex >= 3)
        {
            ESP_LOGI(TAG, "Invalid dataset index: %d\n", datasetIndex);
            return;
        }

        if (hour < 0 || hour >= 24)
        {
            ESP_LOGI(TAG, "Invalid hour: %d\n", hour);
            return;
        }

        _dataSets[datasetIndex].data[hour] = newValue;
        //ESP_LOGI(TAG, "Dataset %d, hour %d updated to %d", datasetIndex, hour, newValue);

        // If the updated dataset is currently displayed, refresh the chart
        if (datasetIndex == _currentDataSetIndex && _chartSeries && _chart)
        {
            lv_chart_set_value_by_id(_chart, _chartSeries, hour, newValue);
        }
    }

    void updateSettingsTextArea(std::string_view newLine)
    {
        if (!_settingsTextArea)
        {
            ESP_LOGE(TAG, "Settings text area is not initialized");
            return;
        }

        // Get the current text in the text area
        const char *currentText = lv_textarea_get_text(_settingsTextArea);
        std::string currentContent(currentText); // Convert to std::string for manipulation

        // Calculate the new length
        size_t newLength = currentContent.size() + newLine.size() + 1; // +1 for '\n'

        // Check if the new length exceeds 100 characters
        if (newLength > 100)
        {
            ESP_LOGI(TAG, "Text area length exceeded 100 characters. Clearing text area.");
            currentContent.clear(); // Clear the current content
        }

        // Append the new line
        currentContent.append(newLine).append("\n");

        // Update the text area with the new content
        lv_textarea_set_text(_settingsTextArea, currentContent.c_str());
    }

private:
    void updateLabel(lv_obj_t *label, int value)
    {
        lv_label_set_text(label, formatPower(value).c_str());
    }

    void showEnergyBar()
    {
        if (_energyBarFrame)
        {
            lv_obj_set_style_bg_color(_energyBarFrame, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(_energyBarFrame, LV_OPA_COVER, LV_PART_MAIN);
        }

        if (_energyBar)
        {
            lv_obj_clear_flag(_energyBar, LV_OBJ_FLAG_HIDDEN);
        }

        if (_energyBarLabel)
        {
            lv_obj_clear_flag(_energyBarLabel, LV_OBJ_FLAG_HIDDEN);
        }

        if (_messageIcon)
        {
            lv_obj_add_flag(_messageIcon, LV_OBJ_FLAG_HIDDEN);
        }
        if (_messageLabel)
        {
            lv_obj_add_flag(_messageLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void showEnergyMessage()
    {

        if (_energyBarFrame)
        {
            lv_obj_set_style_bg_color(_energyBarFrame, lv_color_hex(0xFFCCCC), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(_energyBarFrame, LV_OPA_COVER, LV_PART_MAIN);
        }

        if (_energyBar)
        {
            lv_obj_add_flag(_energyBar, LV_OBJ_FLAG_HIDDEN);
        }

        if (_energyBarLabel)
        {
            lv_obj_add_flag(_energyBarLabel, LV_OBJ_FLAG_HIDDEN);
        }

        if (_messageIcon)
        {
            lv_obj_clear_flag(_messageIcon, LV_OBJ_FLAG_HIDDEN);
        }
        if (_messageLabel)
        {
            lv_obj_clear_flag(_messageLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }
};
