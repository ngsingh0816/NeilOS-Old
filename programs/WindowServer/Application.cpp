//
//  Application.cpp
//  product
//
//  Created by Neil Singh on 5/24/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "Application.h"

#include "Desktop.h"
#include "NSEventDefs.cpp"

#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>

using std::string;
using std::vector;

namespace Application {
	void SetActiveApplication(uint32_t pid);
	bool SendActiveEvent(NSEvent* event);
	bool SendEvent(NSEvent* event, uint32_t pid);
	
	App* GetApplication(uint32_t pid);
	
	vector<App*> applications;
	App* active_app = NULL;
	uint32_t active_fd = -1;
}

void Application::SetActiveApplication(uint32_t pid) {
	if (active_app && active_app->pid == pid)
		return;
	
	bool found = false;
	for (auto app : applications) {
		if (app->pid == pid) {
			found = true;
			active_app = app;
			break;
		}
	}
	if (!found) {
		active_app = NULL;
		return;
	}
	
	if (active_fd != (uint32_t)-1)
		close(active_fd);
	string p = string("/").append(std::to_string(pid));
	active_fd = mq_open(p.c_str(), O_WRONLY);
}

bool Application::SendActiveEvent(NSEvent* event) {
	uint32_t len = 0;
	uint8_t* bytes = event->Serialize(&len);
	if (!bytes)
		return false;
	mq_send(active_fd, reinterpret_cast<const char*>(bytes), len, MQ_PRIO_MAX/2);
	delete[] bytes;
	return true;
}

void Application::RegisterApplication(uint32_t pid, string name, string path) {
	App* app = new App;
	app->pid = pid;
	app->name = name;
	app->path = path;
	applications.push_back(app);
	
	Desktop::RegisterApplication(app);
	
	SetActiveApplication(pid);
	
	// Send all settings info to the application
	NSEventPixelScalingFactor* psf_event = NSEventPixelScalingFactor::Create(Desktop::GetPixelScalingFactor());
	SendActiveEvent(psf_event);
	delete psf_event;
	
	NSEventInitResp* resp_event = NSEventInitResp::Create();
	SendActiveEvent(resp_event);
	delete resp_event;
}

void Application::UnregisterApplication(uint32_t pid) {
	if (active_app && active_app->pid == pid)
		SetActiveApplication(0);
	for (unsigned int z = 0; z < applications.size(); z++) {
		if (applications[z]->pid == pid) {
			Desktop::UnregisterApplication(applications[z]);
			delete applications[z];
			applications.erase(applications.begin() + z);
			break;
		}
	}
}

bool Application::SendEvent(NSEvent* event, uint32_t pid) {
	int fd = mq_open(std::string("/").append(std::to_string(pid)).c_str(), O_WRONLY);
	if (!fd)
		return false;
	
	uint32_t len = 0;
	uint8_t* bytes = event->Serialize(&len);
	if (!bytes)
		return false;
	mq_send(fd, reinterpret_cast<const char*>(bytes), len, MQ_PRIO_MAX/2);
	delete[] bytes;
	
	mq_close(fd);
	
	return true;
}

void Application::SendPixelScalingFactorEvent(float psf) {
	NSEventPixelScalingFactor* event = NSEventPixelScalingFactor::Create(psf);
	
	for (auto a : applications)
		SendEvent(event, a->pid);
}

Application::App* Application::GetApplication(uint32_t pid) {
	for (auto& i : applications) {
		if (i->pid == pid)
			return i;
	}
	return NULL;
}

void Application::RegisterWindow(uint32_t pid, uint32_t wid) {
	App* app = GetApplication(pid);
	app->windows.push_back(wid);
}

void Application::UnregisterWindow(uint32_t pid, uint32_t wid) {
	App* app = GetApplication(pid);
	for (unsigned int z = 0; z < app->windows.size(); z++) {
		if (app->windows[z] == wid) {
			app->windows.erase(app->windows.begin() + z);
			return;
		}
	}
}
