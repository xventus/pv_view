
//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   web_task.cpp
/// @author Petr Vanek

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "application.h"
#include "web_task.h"
#include "http_server.h"
#include "key_val.h"
#include "content_file.h"
#include "http_request.h"
#include <cJSON.h>
#include "utils.h"

WebTask::WebTask()
{
	_queue = xQueueCreate(2, sizeof(int));
	_queueAP = xQueueCreate(2, sizeof(APInfo));
}

WebTask::~WebTask()
{
	done();
	if (_queue)
		vQueueDelete(_queue);
	if (_queueAP)
		vQueueDelete(_queueAP);
	
}

void WebTask::loop()
{

	APInfo apinf = {
		.ap_name = "",
		.rssi = 0
	};
	HttpServer server;
	std::string apinfo;

    Application::getInstance()->signalTaskStart(Application::TaskBit::Web);

	while (true)
	{ // Loop forever
		
		// AP info update - used only in registration
		auto resap = xQueueReceive(_queueAP, (void *)&apinf, 0);
		if (resap == pdTRUE)
		{  
			if (apinfo.size() > 100) {
    			apinfo.clear(); 
			}

			std::string ap_name_to_find = apinf.ap_name;
    		if (apinfo.find(ap_name_to_find) == std::string::npos) {

				ESP_LOGI(TAG, "APINF: %s   rssi:%d", apinf.ap_name, apinf.rssi);
				apinfo += "<li>";
				apinfo += apinf.ap_name;
				apinfo += "  RSSI: ";
				apinfo += std::to_string(apinf.rssi);
				apinfo += "</li>";
			}
		}
		
		
		int receivedMode;
		auto res = xQueueReceive(_queue, (void *)&receivedMode, 0);
		if (res == pdTRUE)
		{
			Mode mode = static_cast<Mode>(receivedMode);
			if (mode == Mode::Stop)
			{
				ESP_LOGI(TAG,"http server mode -> stop");
				server.stop();
			}
			else if (mode == Mode::Setting)
			{
				server.stop();
				server.start();

				// AP info
				server.registerUriHandler("/log", HTTP_GET, [&apinfo](httpd_req_t *req) -> esp_err_t
										  {	
							httpd_resp_set_type(req, "text/plain");
							httpd_resp_send(req, apinfo.c_str() , apinfo.length());
					 		
                    		return ESP_OK; });
			


				// AP main page
				server.registerUriHandler("/", HTTP_GET, [&apinfo](httpd_req_t *req) -> esp_err_t {
						

						auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
						if(from.empty())
							return;
						size_t start_pos = 0;
						while((start_pos = str.find(from, start_pos)) != std::string::npos) {
							str.replace(start_pos, from.length(), to);
							start_pos += to.length(); 
						}
					};

					ConentFile apcnt(literals::kv_fl_ap);	
					KeyVal& kv = KeyVal::getInstance();			  
					auto contnetb = apcnt.readContnet();
					
					if (contnetb.empty()) {
						    httpd_resp_set_type(req, "text/html");
    						const char* error_message = "Initialize SPIF first! - ap.html";
    						httpd_resp_send(req, error_message, HTTPD_RESP_USE_STRLEN);
							return ESP_OK;;
					}

					replaceAll(contnetb, "%SSID%", kv.readString(literals::kv_ssid,"myAP"));
					replaceAll(contnetb, "%PASS%", kv.readString(literals::kv_passwd,"password"));
					replaceAll(contnetb, "%IP%", kv.readString(literals::kv_ip,""));
					replaceAll(contnetb, "%MASK%", kv.readString(literals::kv_mask,""));
					replaceAll(contnetb, "%GW%", kv.readString(literals::kv_gtw,""));
					replaceAll(contnetb, "%DNS%", kv.readString(literals::kv_dns,""));
					replaceAll(contnetb, "%MQTT_BROKER%", kv.readString(literals::kv_mqtt,"192.168.2.20"));
					replaceAll(contnetb, "%BROKER_USER%", kv.readString(literals::kv_user,""));
					replaceAll(contnetb, "%BROKER_PASSWD%", kv.readString(literals::kv_passwdbr,""));
					replaceAll(contnetb, "%TOPIC%", kv.readString(literals::kv_topic,"solax/data"));
					replaceAll(contnetb, "%TIMEZONE%", kv.readString(literals::kv_timezone,literals::kv_def_timezone));
					replaceAll(contnetb, "%TIMESERVER%", kv.readString(literals::kv_timeserver,literals::kv_def_timeserver));

 					httpd_resp_send(req, contnetb.c_str(), contnetb.length());

                    return ESP_OK; });

				server.registerUriHandler("/style.css", HTTP_GET, [](httpd_req_t *req) -> esp_err_t
										  {
					 httpd_resp_set_type(req, "text/css");
					 ConentFile ap1cnt(literals::kv_fl_style);
    				 auto contnet = ap1cnt.readContnet();
                    httpd_resp_send(req, contnet.c_str(), contnet.length());
                   
                    return ESP_OK; });

				// AP setting answer
				server.registerUriHandler("/", HTTP_POST, [](httpd_req_t *req) -> esp_err_t {
					const auto szBuf = 300;
					 std::unique_ptr<char[], std::default_delete<char[]>> content(new char[szBuf]());
					int received = httpd_req_recv(req, content.get(), szBuf - 1);
					ESP_LOGI(TAG,"POST received %d", received);

					if (received >= szBuf - 1) {
						ESP_LOGE(TAG, "Received data exceeds buffer size");
						httpd_resp_send_500(req); 
						return ESP_FAIL;
					}

					if (received <= 0) { 
						if (received == HTTPD_SOCK_ERR_TIMEOUT) {
							httpd_resp_send_408(req);
						}
						return ESP_FAIL;
					}

					// parse response & store configuration
					std::map<std::string, std::string> formData = HttpReqest::parseFormData(std::string(content.get()));
					KeyVal& kv = KeyVal::getInstance();

					kv.writeString(literals::kv_ssid, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_ssid)).c_str());
					kv.writeString(literals::kv_passwd, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_passwd)).c_str());
					kv.writeString(literals::kv_ip, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_ip)).c_str());
					kv.writeString(literals::kv_mask, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_mask)).c_str());		
					kv.writeString(literals::kv_gtw, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_gtw)).c_str());
					kv.writeString(literals::kv_dns, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_dns)).c_str());

					kv.writeString(literals::kv_mqtt, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_mqtt)).c_str());
					kv.writeString(literals::kv_user, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_user)).c_str());
					kv.writeString(literals::kv_passwdbr, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_passwdbr)).c_str());
					kv.writeString(literals::kv_topic, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_topic)).c_str());

					kv.writeString(literals::kv_timezone, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_timezone)).c_str());
					kv.writeString(literals::kv_timeserver, Utils::urlDecode(HttpReqest::getValue(formData, literals::kv_timeserver)).c_str());
									
					// Response
					auto respContnetDialog = std::make_unique<ConentFile>(literals::kv_fl_finish);	
    				const auto finish = respContnetDialog->readContnet();
					if (finish.empty())  {
						httpd_resp_set_type(req, "text/html");
    					const char* error_message = "Initialize SPIF first! -  finish.html";
    					httpd_resp_send(req, error_message, HTTPD_RESP_USE_STRLEN);
					} else {
						httpd_resp_send(req, finish.c_str(), finish.length());
					}
					
					Application::getInstance()->getResetTask()->reset();

					return ESP_OK; 
				});
			}
		}

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void WebTask::apInfo(const APInfo &ap)
{
	if (_queueAP)
	{
		xQueueSendToBack(_queueAP, (void *)&ap, 0);
	}
}

void WebTask::command(Mode mode)
{
	if (_queue)
	{
		int modeToSend = static_cast<int>(mode);
		xQueueSendToBack(_queue, (void *)&modeToSend, 0);
	}
}

