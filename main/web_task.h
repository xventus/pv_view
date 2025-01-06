//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   web_task.h
/// @author Petr Vanek

#pragma once

#include "hardware.h"
#include "rptask.h"
#include "access_point.h"
#include "literals.h"
#include "wifi_scanner.h"


class WebTask : public RPTask
{
public:
 enum class Mode {
		ClearAPInfo,
	    Setting,     	
        Stop       };

	WebTask();
	virtual ~WebTask();
	void command(Mode mode);
	void apInfo(const APInfo& ap);

protected:
	void loop() override;

private:
	static constexpr const char *TAG = "WebTask";

	Mode            _mode {Mode::Stop};
	QueueHandle_t 	_queue;
	QueueHandle_t 	_queueAP;

};
