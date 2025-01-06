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
#include "wifi_client.h"
#include "literals.h"
#include "connection_manager.h"
#include "wifi_scanner.h"


class WifiTask : public RPTask
{
public:
 enum class Mode {
	    AP,     // Access Point 
        Client, // STA
        Stop    // Stopped
    };

	WifiTask();
	virtual ~WifiTask();
	void switchMode(Mode mode);
	bool init(std::shared_ptr<ConnectionManager> connMgr, const char * name, UBaseType_t priority = tskIDLE_PRIORITY, const configSTACK_DEPTH_TYPE stackDepth = configMINIMAL_STACK_SIZE);

protected:
	void loop() override;

private:
	void processMessage(const Mode& mode, WiFiAccessPoint& wftt, WiFiClient& wfcli, esp_netif_ip_info_t &staticip);
	std::string createClientConnectedMessage(bool connect, uint8_t aid, const uint8_t mac[6]) ;

private:
	WiFiScanner 	_scanner;
	Mode            _mode {Mode::Stop};
	QueueHandle_t 	_queue;
	std::shared_ptr<ConnectionManager> _connectionManager;
	static constexpr const char *TAG = "WifiTask";
};
