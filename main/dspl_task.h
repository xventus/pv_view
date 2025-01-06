//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   led_task.h
/// @author Petr Vanek

#pragma once

#include "hardware.h"
#include "rptask.h"
#include "display_driver.h"
#include "dashboard.h"
#include "connection_manager.h"
#include "mqtt_queue_data.h"
#include "shoelace.h"

class DisplayTask : public RPTask
{
public:

	enum class Contnet {
		NoWifi,			// error mesage - wifi
		NoMqtt, 		// error message - mqtt
		Running, 		// wifi & mqtt - ok - energy bar displayed
		UpdateData		// update container for setting view
	};

	// Update message which I will send to the screen
	struct ReqData  {
		Contnet contnet;  
		char msg[50]; 
	};

	DisplayTask();
	virtual ~DisplayTask();
	void settingMsg(std::string_view msg);
	void updateUI(const SolaxParameters& msg);
	bool init(std::shared_ptr<ConnectionManager> connMgr, const char *name, UBaseType_t priority, const configSTACK_DEPTH_TYPE stackDepth);

protected:
	void loop() override;

private:
	static constexpr const char *TAG = "DisplayTask";
	QueueHandle_t 	_queue;
	QueueHandle_t 	_queueData;
	DisplayDriver _dd;
    Dashboard _dashboard;
	std::shared_ptr<ConnectionManager> _connectionManager;
	SolaxParameters  _SolaxData;
	Shoelace		 _consumption;
	Shoelace         _photovoltaic;
	
};
