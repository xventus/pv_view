
//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   wifi_task.cpp
/// @author Petr Vanek

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <sstream>
#include <iomanip>

#include "wifi_task.h"
#include "key_val.h"
#include "literals.h"
#include "application.h"


WifiTask::WifiTask()
{
	_queue = xQueueCreate(2, sizeof(int));
}

WifiTask::~WifiTask()
{
	done();
	if (_queue)
		vQueueDelete(_queue);
}


std::string WifiTask::createClientConnectedMessage(bool connect, uint8_t aid, const uint8_t mac[6]) {
    std::ostringstream oss; // Create a string stream
    oss << (connect ? "Client connected:" : "Client disconnected:") << "AID=" << static_cast<int>(aid) << ", MAC=";
    oss << std::hex << std::uppercase << std::setfill('0'); 
    for (int i = 0; i < 6; ++i) {
        oss << std::setw(2) << static_cast<int>(mac[i]);
        if (i < 5) oss << ":"; 
    }
    return oss.str(); 
}

void WifiTask::loop()
{

	WiFiAccessPoint wftt;
	WiFiClient wfcli;
	esp_netif_ip_info_t staticip = {
		.ip = {.addr = 0},
		.netmask = {.addr = 0},
		.gw = {.addr = 0}};
	bool processit = true;
	int receivedMode = static_cast<int>(Mode::Stop);

	//wifi connection lambdas --->
	WiFiConnectedCallback ConnCallback = [this](const ip_event_got_ip_t &event)
	{
		ESP_LOGI(TAG, "WiFi Connected!");
		ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&event.ip_info.ip));
		ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event.ip_info.netmask));
		ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event.ip_info.gw));
		std::ostringstream oss;

		// Convert IP address
		oss << "IP: " << (event.ip_info.ip.addr & 0xFF) << "."
			<< ((event.ip_info.ip.addr >> 8) & 0xFF) << "."
			<< ((event.ip_info.ip.addr >> 16) & 0xFF) << "."
			<< ((event.ip_info.ip.addr >> 24) & 0xFF);

		// Add gateway
		oss << " GTW: " << (event.ip_info.gw.addr & 0xFF) << "."
			<< ((event.ip_info.gw.addr >> 8) & 0xFF) << "."
			<< ((event.ip_info.gw.addr >> 16) & 0xFF) << "."
			<< ((event.ip_info.gw.addr >> 24) & 0xFF);

		std::string ms = oss.str();

		// Send message
		Application::getInstance()->getDisplayTask()->settingMsg(ms.c_str());

		if (_connectionManager)
			_connectionManager->setConnected();
	};

	WiFiDisconnectedCallback DiscCallback = [this]()
	{
		ESP_LOGW(TAG, "WiFi Disconnected!");
		Application::getInstance()->getDisplayTask()->settingMsg("Disconnected");
		if (_connectionManager)
			_connectionManager->setDisconnected();
	};

	WiFiApConnectedCallback APConnCallback =  [this](uint8_t* mac, uint8_t aid) {
		auto s = createClientConnectedMessage(true, aid, mac);
    	ESP_LOGW(TAG, "%s", s.c_str());
		Application::getInstance()->getDisplayTask()->settingMsg(s);
	};

	WiFiApDisconnectedCallback APDiscCallback =  [this](uint8_t* mac, uint8_t aid) {
    	auto s = createClientConnectedMessage(false, aid, mac);
    	ESP_LOGW(TAG,"%s" ,s.c_str());
		Application::getInstance()->getDisplayTask()->settingMsg(s);
	};

	wfcli.registerDisconnectedCallback(DiscCallback);
	wfcli.registerConnectedCallback(ConnCallback);
	wftt.registerClientConnectedCallback(APConnCallback);
	wftt.registerClientDisconnectedCallback(APDiscCallback);
	
	// <------

	Application::getInstance()->signalTaskStart(Application::TaskBit::WiFi);

	while (true)
	{ // Loop forever

		auto res = xQueueReceive(_queue, (void *)&receivedMode, 0);
		if (res == pdTRUE)
		{
			processit = true;
		}

		if (processit) {
			processMessage(static_cast<Mode>(receivedMode), wftt, wfcli, staticip);
			processit = false;
		}
		

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}


void WifiTask::processMessage(const Mode& mode, WiFiAccessPoint& wftt,	WiFiClient& wfcli, esp_netif_ip_info_t &staticip)
{
			// Stop mode & check initial configuration
			if (mode == Mode::Stop)
			{
				ESP_LOGI(TAG, "All STOP");
				// stop all previous wifi modes
				wftt.stop();
				if (wfcli.isConnected())
				{
					wfcli.disconnect();
				}

				if (_connectionManager)
					_connectionManager->setDisconnected();
			}
			else if (mode == Mode::Client)
			{
				ESP_LOGI(TAG, "CLIENT MODE");
				wftt.stop();
				if (wfcli.isConnected())
				{
					wfcli.disconnect();
				}

				wfcli.init(false);

				KeyVal &kv = KeyVal::getInstance();
				esp_netif_str_to_ip4(kv.readString(literals::kv_ip).c_str(), &staticip.ip);
				esp_netif_str_to_ip4(kv.readString(literals::kv_mask).c_str(), &staticip.netmask);
				esp_netif_str_to_ip4(kv.readString(literals::kv_gtw).c_str(), &staticip.gw);

				bool cntok = false;
				if (staticip.ip.addr == 0 || staticip.netmask.addr == 0)
				{
					// DHCP mode
					cntok = wfcli.connect(kv.readString(literals::kv_ssid), kv.readString(literals::kv_passwd), true, nullptr);
				}
				else
				{
					// static IP mode
					cntok = wfcli.connect(kv.readString(literals::kv_ssid), kv.readString(literals::kv_passwd), false, &staticip);
				}

				// no nedded web interface
				Application::getInstance()->getWebTask()->command(WebTask::Mode::Stop);
			}
			else if (mode == Mode::AP)
			{
				ESP_LOGI(TAG, "AP MODE");
				if (wfcli.isConnected())
				{
					wfcli.disconnect();
				}
			
				if (_connectionManager) {
				 while(	_connectionManager->isAPActive()) {
					vTaskDelay(500 / portTICK_PERIOD_MS);
				 }
				}

				// scan APs for info
				
				_scanner.init(false);
				_scanner.scan();

				// clear web task AP info
				Application::getInstance()->getWebTask()->command(WebTask::Mode::ClearAPInfo);
				ScanResultCallback callback = [](const APInfo &info)
				{
					// send each AP into WebTask
					Application::getInstance()->getWebTask()->apInfo(info);
				};
				_scanner.processResults(callback);
				_scanner.down();
				// AP start
				std::string a = "AP started: ";
				a += literals::ap_name;

				Application::getInstance()->getDisplayTask()->settingMsg(a);
				wftt.start(literals::ap_name, literals::ap_passwd);
				Application::getInstance()->getWebTask()->command(WebTask::Mode::Setting);
				
			}
		}

void WifiTask::switchMode(Mode mode)
{
	if (_queue)
	{
		int modeToSend = static_cast<int>(mode);
		xQueueSendToBack(_queue, (void *)&modeToSend, 0);
	}
}

bool WifiTask::init(std::shared_ptr<ConnectionManager> connMgr, const char *name, UBaseType_t priority, const configSTACK_DEPTH_TYPE stackDepth)
{
	bool rc = false;
	_connectionManager = connMgr;
	rc = RPTask::init(name, priority, stackDepth);
	return rc;
}