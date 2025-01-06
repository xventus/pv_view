//
// vim: ts=4 et
// Copyright (c) 2023 Petr Vanek, petr@fotoventus.cz
//
/// @file   reset_task.h
/// @author Petr Vanek

#pragma once

#include "hardware.h"
#include "rptask.h"



class ResetTask : public RPTask
{
public:
	ResetTask();
	virtual ~ResetTask();
	void reset();

protected:
	void loop() override;

private:
	static constexpr const char *LOG_TAG = "RESET";
	QueueHandle_t 	_queue;
};
