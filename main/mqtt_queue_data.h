//
// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   mqtt_queue_data.h
/// @author Petr Vanek

#pragma once

#include <ctype.h>



struct SolaxParameters {
    int32_t PvVoltage1{0};
    int32_t PvVoltage2{0};
    int32_t PvCurrent1{0};
    int32_t PvCurrent2{0};
    int32_t Powerdc1{0};
    int32_t Powerdc2 {0};
    int32_t BatVoltage_Charge1{0};
    int32_t BatCurrent_Charge1{0};
    int32_t Batpower_Charge1{0};
    int32_t TemperatureBat{0};
    int32_t BattCap{0};
    int32_t FeedinPower{0};
    int32_t GridPower_R{0};
    int32_t GridPower_S{0};
    int32_t GridPower_T{0};
    int32_t Etoday_togrid{0};
    int32_t Temperature{0};
    int32_t RunMode{0};
    int32_t BDCStatus{0};
    int32_t GridStatus{0};
    int32_t MPPTCount{0};
    int32_t Hdo{0};
};
