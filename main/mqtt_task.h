//
// vim: ts=4 et
// Copyright (c) 2024 Petr Vanek, petr@fotoventus.cz
//
/// @file   mqtt_task.h
/// @author Petr Vanek

#pragma once

#include <memory>

#include "hardware.h"
#include "rptask.h"
#include "mqtt.h"
#include "literals.h"
#include "connection_manager.h"
#include  "mqtt_queue_data.h"
#include "mqtt_queue_data.h"

class MqttTask : public RPTask
{
public:
 	MqttTask();
	virtual ~MqttTask();
	bool init(std::shared_ptr<ConnectionManager> connMgr, const char * name, UBaseType_t priority = tskIDLE_PRIORITY, const configSTACK_DEPTH_TYPE stackDepth = configMINIMAL_STACK_SIZE);

protected:
	void loop() override;
	void initializeMqttClient();
	void doneMqttClient();

private:
	static constexpr const char *LOG_TAG = "MqttTask";
	std::shared_ptr<ConnectionManager> _connectionManager;
	// Unique pointer to the MQTT client object
    std::unique_ptr<Mqtt> _mqttClient {};
    // Boolean flag to track if the client is connected
	 bool _mqttInitialized{false};
	 SolaxParameters _solaxData;

};
