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
#include <numeric>
#include "esp_log.h"
#include "display_driver.h"
#include "icons8_solar_panel_48.h"
#include "icons8_car_battery_48.h"
#include "icons8_telephone_pole_48.h"
#include "icons8_home_48.h"
#include "icons8_generator_32.h"
#include "icons8_error_30.h"
#include "icons8_wifi_24.h"
#include "utils.h"

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
    lv_obj_t *_maxLabel{nullptr};        // Maximum value for chart
    lv_obj_t *_barGraphFrame{nullptr};   //

    lv_obj_t *_totalSolLabel{nullptr};
    lv_obj_t *_dayConsumpLabel{nullptr};
    lv_obj_t *_timeLabel{nullptr};
    lv_obj_t *_dateLabel{nullptr};
    lv_obj_t *_totalCons{nullptr};
    lv_obj_t *_totalSol{nullptr};

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
        {"Photovoltaic Yield", lv_color_hex(0xFF4500), {LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE}},
        {"Energy Consumption", lv_color_hex(0x007BFF), {LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE, LV_CHART_POINT_NONE}}};

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
            "No MQTT"};
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
            lv_obj_align(_messageIcon, LV_ALIGN_TOP_MID, 0, 0);
        }
        if (!_messageLabel)
        {
            _messageLabel = lv_label_create(_energyBarFrame);
            lv_obj_set_style_text_font(_messageLabel, &lv_font_montserrat_14, 0);
            lv_obj_align(_messageLabel, LV_ALIGN_BOTTOM_MID, 0, 0);
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
        const int frameSolWidth = 157;   // solar, batt
        const int frameSolHeight = 120;  // solar, batt
        const int frameGridWidth = 157;  // grid, home
        const int frameGridHeight = 100; // grid, home
        const int distHoziz = 5;
        const int distVert = 4;
        const int dist = 2;
        const int distIcon = 15; // grid, home
        const int distIconSol = 30;
        const int energyProgressWidth = 100;
        const int frameOverHeight = 100;

        // Solar Panels
        _solarPanelFrame = createFrame(frameSolWidth, frameSolHeight, LV_ALIGN_TOP_LEFT, dist, dist, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _solarPanelTotalPowerLabel = addLabel(_solarPanelFrame, "1000 W", LV_ALIGN_TOP_MID, 0, -dist, &lv_font_montserrat_16);
        addIcon(_solarPanelFrame, &icons8_solar_panel_48, LV_ALIGN_CENTER, 0, distIconSol);
        _solarPanelString1Label = addLabel(_solarPanelFrame, "520 W", LV_ALIGN_CENTER, -30, -10, &lv_font_montserrat_12);
        _solarPanelString2Label = addLabel(_solarPanelFrame, "480 W", LV_ALIGN_CENTER, 30, -10, &lv_font_montserrat_12);
        lv_obj_set_scrollbar_mode(_solarPanelFrame, LV_SCROLLBAR_MODE_OFF);

        // Battery
        _batteryFrame = createFrame(frameSolWidth, frameSolHeight, LV_ALIGN_TOP_RIGHT, -dist, dist, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _batteryPercentageLabel = addLabel(_batteryFrame, "80%", LV_ALIGN_TOP_MID, 0, -dist, &lv_font_montserrat_16);
        addIcon(_batteryFrame, &icons8_car_battery_48, LV_ALIGN_CENTER, 0, distIconSol);
        _batteryPowerLabel = addLabel(_batteryFrame, "500 W", LV_ALIGN_CENTER, -30, -10, &lv_font_montserrat_12);
        _batteryTempLabel = addTemperatureLabel(_batteryFrame, "35.0 °C", LV_ALIGN_CENTER, 30, -10);
        lv_obj_set_scrollbar_mode(_batteryFrame, LV_SCROLLBAR_MODE_OFF);

        // grid
        _gridFrame = createFrame(frameGridWidth, frameGridHeight, LV_ALIGN_TOP_LEFT, dist, frameSolHeight + distVert, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _gridLabel = addLabel(_gridFrame, "781 W", LV_ALIGN_TOP_MID, 0, -dist, &lv_font_montserrat_16);
        addIcon(_gridFrame, &icons8_telephone_pole_48, LV_ALIGN_CENTER, 0, distIcon);
        _gridLed = lv_led_create(_gridFrame);
        lv_obj_set_size(_gridLed, 5, 5); // Set LED size
        lv_obj_align(_gridLed, LV_ALIGN_BOTTOM_LEFT, 5, -5);
        lv_obj_set_scrollbar_mode(_gridFrame, LV_SCROLLBAR_MODE_OFF);

        // Consumption - home
        _consumptionFrame = createFrame(frameGridWidth, frameGridHeight, LV_ALIGN_TOP_RIGHT, -dist, frameSolHeight + distVert, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _consumptionLabel = addLabel(_consumptionFrame, "532 W", LV_ALIGN_TOP_MID, 0, -dist, &lv_font_montserrat_16);
        addIcon(_consumptionFrame, &icons8_home_48, LV_ALIGN_CENTER, 0, distIcon);
        lv_obj_set_scrollbar_mode(_consumptionFrame, LV_SCROLLBAR_MODE_OFF);

        // Overview
        _overviewFrame = createFrame(frameGridWidth, frameOverHeight, LV_ALIGN_TOP_LEFT, dist, frameSolHeight + distHoziz + dist + frameGridHeight, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _overviewPowerLabel = addLabel(_overviewFrame, "3500 W", LV_ALIGN_TOP_MID, 0, -dist, &lv_font_montserrat_16);
        _overviewTempLabel = addTemperatureLabel(_overviewFrame, "50.0 °C", LV_ALIGN_BOTTOM_MID, 0, 5);
        addIcon(_overviewFrame, &icons8_generator_32, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_scrollbar_mode(_overviewFrame, LV_SCROLLBAR_MODE_OFF);

        // Energy Bar
        _energyBarFrame = createFrame(frameGridWidth, frameOverHeight, LV_ALIGN_TOP_RIGHT, -dist, frameSolHeight + distHoziz + dist + frameGridHeight, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        _energyBarLabel = addLabel(_energyBarFrame, "1200 W", LV_ALIGN_TOP_MID, 0, -dist, &lv_font_montserrat_16);
        _energyBar = addBar(_energyBarFrame, energyProgressWidth, 20, LV_ALIGN_BOTTOM_MID, 0, -10);

        createBarGraph("", lv_color_hex(0x007BFF));

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

    lv_obj_t *addIcon(lv_obj_t *parent, const lv_img_dsc_t *icon, lv_align_t align, int x, int y)
    {
        lv_obj_t *img = lv_img_create(parent);
        lv_img_set_src(img, icon);
        lv_obj_align(img, align, x, y);
        return img;
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

        int totalPower = string1Power + string2Power;

        lv_label_set_text(_solarPanelTotalPowerLabel, Utils::formatPower(totalPower).c_str());

        lv_label_set_text(_solarPanelString1Label, Utils::formatPower(string1Power).c_str());
        lv_label_set_text(_solarPanelString2Label, Utils::formatPower(string2Power).c_str());

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

        lv_label_set_text(_batteryPowerLabel, Utils::formatPower(power).c_str());

        sprintf(text, "%.1f °C", temp);
        lv_label_set_text(_batteryTempLabel, text);
        updateTemperatureTextColor(_batteryTempLabel, temp);

        // Change the frame color based on power
        if (power == 0)
        {
            // Black for zero power
            lv_obj_set_style_border_color(_batteryFrame, lv_color_hex(0x000000), 0);
        }
        else if (power > 100)
        {
            // Dark green for positive power
            lv_obj_set_style_border_color(_batteryFrame, lv_color_hex(0x00EE00), 0);
        }
        else if (power < 100)
        {
            // Dark red for negative power
            lv_obj_set_style_border_color(_batteryFrame, lv_color_hex(0xEE0000), 0);
        }
    }

    void updateConsumption(int power)
    {
        updateLabel(_consumptionLabel, power);
    }

    void updateGrid(int power, bool ongrid)
    {
        updateLabel(_gridLabel, power);

        if (ongrid)
        {
            lv_led_set_color(_gridLed, lv_color_hex(0x00FF00)); // Green for online
            lv_led_on(_gridLed);                                // Turn the LED on
        }
        else
        {
            lv_led_set_color(_gridLed, lv_color_hex(0xFF0000)); // Red for offline
            lv_led_on(_gridLed);                                // Turn the LED on
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

    void graphDisplay(bool hideGraph)
    {
        if (hideGraph)
        {
            lv_obj_add_flag(_chartTitleLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_chart, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_maxLabel, LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(_totalSolLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_dayConsumpLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_timeLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_dateLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_totalSol, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_totalCons, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(_chartTitleLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_chart, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(_maxLabel, LV_OBJ_FLAG_HIDDEN);

            lv_obj_add_flag(_totalSolLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_dayConsumpLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_timeLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_dateLabel, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_totalSol, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_totalCons, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void createBarGraph(const char *title, lv_color_t barColor)
    {
        // Create a frame for the bar graph
        _barGraphFrame = createFrame(315, 148, LV_ALIGN_BOTTOM_MID, 0, -2, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        lv_obj_set_scrollbar_mode(_barGraphFrame, LV_SCROLLBAR_MODE_OFF);

        // Add a title label to the frame
        _chartTitleLabel = addLabel(_barGraphFrame, title, LV_ALIGN_TOP_MID, 0, -10, &lv_font_montserrat_14);

        // Create the bar graph
        _chart = lv_chart_create(_barGraphFrame);
        lv_obj_set_size(_chart, 260, 80); // Set the size of the bar graph
        lv_obj_align(_chart, LV_ALIGN_CENTER, 0, 20);

        // Set the Y-axis range
        lv_chart_set_range(_chart, LV_CHART_AXIS_PRIMARY_Y, 10, 10000); // Assuming max value is 12000W

        // Configure the chart as a bar graph
        lv_chart_set_type(_chart, LV_CHART_TYPE_BAR);
        lv_obj_set_style_line_opa(_chart, LV_OPA_TRANSP, LV_PART_ITEMS);
        lv_obj_set_style_line_width(_chart, 0, LV_PART_ITEMS);
        lv_obj_set_style_pad_column(_chart, 0, LV_PART_MAIN);              
        lv_obj_set_style_pad_gap(_chart, 4, LV_PART_MAIN);  
        lv_chart_set_point_count(_chart, 24); // 24 hours of data

        lv_chart_set_div_line_count(_chart, 10, 0); 

        // Remove vertical grid lines
        lv_obj_set_style_pad_column(_chart, 0, LV_PART_MAIN);

        // Create the series and assign it to _chartSeries
        _chartSeries = lv_chart_add_series(_chart, barColor, LV_CHART_AXIS_PRIMARY_Y);

        // Populate the series with initial data
        for (int i = 0; i < 24; i++)
        {
            lv_chart_set_value_by_id(_chart, _chartSeries, i, _dataSets[0].data[i]);
        }

        // Maximum info
        _maxLabel = lv_label_create(lv_obj_get_parent(_chart));
        lv_label_set_text(_maxLabel, "Max: 0 kW");
        lv_obj_set_style_text_font(_maxLabel, &lv_font_montserrat_12, 0);

        lv_obj_set_pos(_maxLabel, lv_obj_get_x(_chart), lv_obj_get_y(_chart) + 20);

        // Make the chart clickable
        lv_obj_add_flag(_chart, LV_OBJ_FLAG_CLICKABLE);

        // Add an event listener for the chart
        lv_obj_add_event_cb(_barGraphFrame, [](lv_event_t *e)
                            {
                Dashboard *dashboard = static_cast<Dashboard *>(lv_event_get_user_data(e));
                ESP_LOGI(TAG,"Chart clicked");
        dashboard->onChartClick(); }, LV_EVENT_CLICKED, this);

        lv_obj_add_event_cb(_chart, [](lv_event_t *e)
                            {
                Dashboard *dashboard = static_cast<Dashboard *>(lv_event_get_user_data(e));
                ESP_LOGI(TAG,"Chart clicked");
        dashboard->onChartClick(); }, LV_EVENT_CLICKED, this);

        // --------- Create overview

        _totalSolLabel = addLabel(_barGraphFrame, "1000 Wh", LV_ALIGN_TOP_LEFT, 0, 20, &lv_font_montserrat_16);
        _totalSol = addIcon(_barGraphFrame, &icons8_solar_panel_48, LV_ALIGN_BOTTOM_LEFT, 0, -15);

        _dayConsumpLabel = addLabel(_barGraphFrame, "1000 Wh", LV_ALIGN_TOP_RIGHT, 0, 20, &lv_font_montserrat_16);
        _totalCons = addIcon(_barGraphFrame, &icons8_home_48, LV_ALIGN_BOTTOM_RIGHT, 0, -15);

        _timeLabel = addLabel(_barGraphFrame, "10:00", LV_ALIGN_CENTER, 0, 0, &lv_font_montserrat_16);
        _dateLabel = addLabel(_barGraphFrame, "10.10.2025", LV_ALIGN_CENTER, 0, 20, &lv_font_montserrat_16);

        graphDisplay(false);
    }

    void updateChartRange()
    {
        if (!_chart || !_chartSeries)
        {
            ESP_LOGE(TAG, "Error: _chart or _chartSeries is null");
            return;
        }

        int minValue = 0;
        int maxValue = 0;

        for (int i = 0; i < 24; i++)
        {
            int value = _dataSets[_currentDataSetIndex].data[i];
            if (value == LV_CHART_POINT_NONE)
                continue;
            if (value < minValue)
                minValue = value;
            if (value > maxValue)
                maxValue = value;
        }

        lv_chart_set_range(_chart, LV_CHART_AXIS_PRIMARY_Y, minValue, maxValue);
        lv_chart_refresh(_chart);

        if (_maxLabel)
        {
            std::string mx = "Max: ";
            if (maxValue == INT16_MIN || minValue == INT16_MAX)
                _maxLabel = 0;
            mx += Utils::formatPower(maxValue, "W", "h");
            lv_label_set_text(_maxLabel, mx.c_str());
        }

       //ESP_LOGI(TAG, "Chart range updated: Min=%d, Max=%d", minValue, maxValue);
       //ESP_LOGI(TAG, "Chart range index: %d\n", _currentDataSetIndex);
    }

    void updateChart()
    {
        if (_currentDataSetIndex == 2)
        {
            graphDisplay(true);
        }
        else
        {
            graphDisplay(false);
            lv_label_set_text(_chartTitleLabel, _dataSets[_currentDataSetIndex].description);
            lv_chart_set_series_color(_chart, _chartSeries, _dataSets[_currentDataSetIndex].color);

            for (int i = 0; i < 24; i++)
            {
                lv_chart_set_value_by_id(_chart, _chartSeries, i, _dataSets[_currentDataSetIndex].data[i]);
            }

            updateChartRange();
            //ESP_LOGI(TAG, "Dataset switched to: %s\n", _dataSets[_currentDataSetIndex].description);
        }
    }

    void onChartClick()
    {
        ESP_LOGI(TAG, "onChartClick called");

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

        _currentDataSetIndex = (_currentDataSetIndex + 1) % 3;
        updateChart();

        ESP_LOGI(TAG, "onChartClick: %d\n", _currentDataSetIndex);
    }

    void clearAllDataSets()
    {
        for (int datasetIndex = 0; datasetIndex < 3; datasetIndex++)
        {
            for (int hour = 0; hour < 24; hour++)
            {
                _dataSets[datasetIndex].data[hour] = LV_CHART_POINT_NONE;
            }
        }
        ESP_LOGI(TAG, "All data sets cleared\n");

        // Optionally update the chart if it's active
        if (_chartSeries && _chart)
        {
            for (int i = 0; i < 24; i++)
            {
                lv_chart_set_value_by_id(_chart, _chartSeries, i, LV_CHART_POINT_NONE);
            }

            updateChart();
            updateChartRange();
        }
    }

    void updateDataSetHour(int datasetIndex, int hour, int newValue)
    {
        if (datasetIndex < 0 || datasetIndex >= 2)
        {
            ESP_LOGI(TAG, "Invalid dataset index: %d\n", datasetIndex);
            return;
        }

        if (hour < 0 || hour >= 24)
        {
            ESP_LOGI(TAG, "Invalid hour: %d\n", hour);
            return;
        }

        _dataSets[datasetIndex].data[hour] = (newValue == 0) ? LV_CHART_POINT_NONE : newValue;

        if (datasetIndex == _currentDataSetIndex && _chartSeries && _chart)
        {
            lv_chart_set_value_by_id(_chart, _chartSeries, hour, newValue);

            for (int i = hour; i < 24; i++)
            {
                lv_chart_set_value_by_id(_chart, _chartSeries, i, LV_CHART_POINT_NONE);
            }
        }

        updateChart();
        updateChartRange();
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

    void updateDateTime(std::string_view date, std::string_view time)
    {

        if (!_timeLabel || !_dateLabel || !lv_obj_is_valid(_timeLabel) || !lv_obj_is_valid(_dateLabel))
        {
            ESP_LOGE(TAG, "Labels _timeLabel or _dateLabel are not valid.");
            return;
        }

        if (_timeLabel && !time.empty())
            lv_label_set_text(_timeLabel, time.data());
        if (_dateLabel && !date.empty())
            lv_label_set_text(_dateLabel, date.data());
    }

    void updateTotal(int sol, int cons)
    {
        if (_totalSolLabel)
            lv_label_set_text(_totalSolLabel, Utils::formatPower(sol, "W", "h").c_str());
        if (_dayConsumpLabel)
            lv_label_set_text(_dayConsumpLabel, Utils::formatPower(cons, "W", "h").c_str());
    }

private:
    void updateLabel(lv_obj_t *label, int value)
    {
        lv_label_set_text(label, Utils::formatPower(value).c_str());
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
