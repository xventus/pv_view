//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   utils.h
/// @author Petr Vanek

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <string_view>
#include <time.h>
#include "esp_sntp.h"
#include "esp_log.h"

/// @brief
class Utils
{
private:
    static constexpr const char *TAG = "UTILS";

public:
    static std::string urlDecode(std::string_view encoded)
    {
        std::ostringstream decoded;

        for (size_t i = 0; i < encoded.size(); ++i)
        {
            if (encoded[i] == '%')
            {

                if (i + 2 < encoded.size() && std::isxdigit(encoded[i + 1]) && std::isxdigit(encoded[i + 2]))
                {
                    std::string hexValue(encoded.substr(i + 1, 2));
                    char decodedChar = static_cast<char>(std::stoi(hexValue, nullptr, 16));
                    decoded << decodedChar;
                    i += 2;
                }
                else
                {

                    decoded << encoded[i];
                }
            }
            else if (encoded[i] == '+')
            {
                decoded << ' ';
            }
            else
            {
                decoded << encoded[i];
            }
        }

        return decoded.str();
    }

    static int getCurrentHour()
    {
        time_t now = time(NULL);
        struct tm localTime;
        localtime_r(&now, &localTime);
        return localTime.tm_hour;
    }
};
