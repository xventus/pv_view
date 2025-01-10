//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   literals.h
/// @author Petr Vanek

#pragma once

#include <stdio.h>
#include <string_view>


class literals
{
public:

    // internal - do not modify
    
    // internal task name
    static constexpr const char *tsk_web{"WEBTSK"};
    static constexpr const char *tsk_wifi{"WIFITSK"};
    static constexpr const char *tsk_rst{"RSTTSK"};
    static constexpr const char *tsk_dspl{"DSPLTSK"};
    static constexpr const char *tsk_mqtt{"DSPLTSK"};
    static constexpr const char *tsk_time{"TIMETSK"};

    // AP definition
    static constexpr const char *ap_name{"PVVIEWAP"};
    static constexpr const char *ap_passwd{""};

    // KV & files
    static constexpr const char *kv_namespace{"pvview"};
    static constexpr const char *kv_ssid{"ssid"};
    static constexpr const char *kv_passwd{"pass"};
    static constexpr const char *kv_ip{"ip"};
    static constexpr const char *kv_gtw{"gw"};
    static constexpr const char *kv_mask{"mask"};
    static constexpr const char *kv_dns{"dns"};
    static constexpr const char *kv_mqtt{"mqtt"};
    static constexpr const char *kv_user{"user"};
    static constexpr const char *kv_passwdbr{"passbr"};
    static constexpr const char *kv_topic{"topic"};
    static constexpr const char *kv_timezone{"timezone"};
    static constexpr const char *kv_timeserver{"timeserver"};
    
    // spiffs filenames
    static constexpr const char *kv_fl_ap{"/spiffs/ap.html"};
    static constexpr const char *kv_fl_style{"/spiffs/style.css"}; 
    static constexpr const char *kv_fl_finish{"/spiffs/finish.html"};
    
    // time 
    static constexpr const char *kv_def_timezone{"CET-1CEST,M3.5.0,M10.5.0/3"};
    static constexpr const char *kv_def_timeserver{"cz.pool.ntp.org"};

};