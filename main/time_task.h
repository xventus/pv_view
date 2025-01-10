//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   time_task.h
/// @author Petr Vanek

#pragma once

#include "hardware.h"
#include "rptask.h"
#include "connection_manager.h"



class TimeTask : public RPTask
{
public:
	TimeTask();
	virtual ~TimeTask();
	bool init(std::shared_ptr<ConnectionManager> connMgr, const char *name, UBaseType_t priority, const configSTACK_DEPTH_TYPE stackDepth);
private:
	void initializeSNTP();
	void updateTimeZone(const char* tmz);
	bool syncTime();
protected:
	void loop() override;

private:
	static constexpr const char *LOG_TAG = "TIME";
	std::shared_ptr<ConnectionManager> _connectionManager;
	std::string _timeserver;

};
