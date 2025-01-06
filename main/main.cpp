//
// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   main.cpp
/// @author Petr Vanek

#include "application.h"

extern "C"
{
    void app_main(void);
}

void app_main(void)
{

    Application::getInstance()->init();
    Application::getInstance()->run();
    return;
  
}

