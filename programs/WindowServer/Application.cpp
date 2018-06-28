//
//  Application.cpp
//  product
//
//  Created by Neil Singh on 5/24/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "Application.h"

#include "Desktop.h"
#include "Events/NSEventDefs.cpp"
#include "Window.h"

#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>

using std::string;
using std::vector;

namespace Application {
	// Events
	void QuitEvent(uint8_t* data, uint32_t length);
	void SetMenuEvent(uint8_t* data, uint32_t length);
	void SetCursorEvent(uint8_t* data, uint32_t length);
	void ContextMenuEvent(uint8_t* data, uint32_t length);
	
	void SetActiveApplication(uint32_t pid);
	bool SendActiveEvent(NSEvent* event);
	bool SendEvent(NSEvent* event, uint32_t pid);
	
	void SetMenuActions(uint32_t pid, NSMenu* menu, std::vector<unsigned int> indices,
						bool is_context, uint32_t menu_id);
	
	App* GetApplication(uint32_t pid);
	
	vector<App*> applications;
	App* active_app = NULL;
	uint32_t active_fd = -1;
}

void Application::ProcessEvent(uint8_t* data, uint32_t length) {
	if (length < sizeof(uint32_t) * 2)
		return;
	
	uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
	if (buffer[0] != EVENT_APPLICATION_ID)
		return;
	
	uint32_t subcode = buffer[1];
	
	switch (subcode) {
		case APPLICATION_EVENT_QUIT:
			QuitEvent(data, length);
			break;
		case APPLICATION_EVENT_SET_MENU:
			SetMenuEvent(data, length);
			break;
		case APPLICATION_EVENT_SET_CURSOR:
			SetCursorEvent(data, length);
			break;
		case APPLICATION_EVENT_CONTEXT_MENU:
			ContextMenuEvent(data, length);
			break;
	}
}

// Events
void Application::QuitEvent(uint8_t* data, uint32_t length) {
	NSEventApplicationQuit* e = NSEventApplicationQuit::FromData(data, length);
	if (!e)
		return;
	
	UnregisterApplication(e->GetPid());
	delete e;
}

void Application::SetMenuActions(uint32_t pid, NSMenu* menu, std::vector<unsigned int> indices,
								 bool is_context, uint32_t menu_id) {
	if (!menu)
		return;
	
	auto items = menu->GetItems();
	for (unsigned int z = 0; z < items.size(); z++) {
		std::vector<unsigned int> i = indices;
		i.push_back(z);
		items[z]->SetAction([pid, i, is_context, menu_id](NSMenuItem* item) {
			bool d = dynamic_cast<NSMenuViewItem*>(item) == NULL;
			NSEventApplicationMenuEvent* e = NSEventApplicationMenuEvent::Create(i, d ? NULL : item,
																				 is_context, menu_id, d);
			Application::SendEvent(e, pid);
			delete e;
		});
		items[z]->SetCursorEvent([](NSCursorRegion* region) {
			NSImage* image = region->GetImage();
			if (image)
				Desktop::SetCursor(image, region->GetHotspot());
			else
				Desktop::SetCursor(region->GetCursor());
		});
		SetMenuActions(pid, items[z]->GetSubmenu(), i, is_context, menu_id);
	}
}

void Application::SetMenuEvent(uint8_t* data, uint32_t length) {
	NSEventApplicationSetMenu* e = NSEventApplicationSetMenu::FromData(data, length);
	if (!e)
		return;
	
	App* app = GetApplication(e->GetPid());
	if (!app)
		return;
	
	if (app->menu)
		delete app->menu;
	app->menu = new NSMenu(e->GetMenu());
	SetMenuActions(e->GetPid(), app->menu, std::vector<unsigned int>(), false, 0);
	
	delete e;
	
	if (active_app == app)
		Desktop::UpdateMenu();
}

void Application::SetCursorEvent(uint8_t* data, uint32_t length) {
	NSEventApplicationSetCursor* e = NSEventApplicationSetCursor::FromData(data, length);
	if (!e)
		return;
	
	if (e->GetImage())
		Desktop::SetCursor(e->GetImage(), e->GetHotspot());
	else
		Desktop::SetCursor(NSCursor::Cursor(e->GetCursor()));
	
	delete e;
}

void Application::ContextMenuEvent(uint8_t* data, uint32_t length) {
	NSEventApplicationContextMenu* e = NSEventApplicationContextMenu::FromData(data, length);
	if (!e)
		return;
	
	NSMenu* menu = NULL;
	if (e->HasMenu()) {
		menu = new NSMenu(e->GetMenu());
		SetMenuActions(e->GetPid(), menu, std::vector<unsigned int>(), true, e->GetID());
	}
	
	Desktop::PopupContextMenu(e->GetPid(), menu, e->GetPoint(), e->GetID());
	
	delete e;
}

void Application::SetActiveApplication(uint32_t pid) {
	if (active_app && active_app->pid == pid)
		return;
	
	if (active_app) {
		NSEventApplicationFocus* e = NSEventApplicationFocus::Create(active_app->pid, false);
		SendActiveEvent(e);
		delete e;
	}
	
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
		mq_close(active_fd);
	string p = string("/").append(std::to_string(pid));
	active_fd = mq_open(p.c_str(), O_WRONLY);
	
	// Find last window to make active
	uint32_t last_wid = Window::FindLastVisibleWindow(pid);
	if (last_wid != (uint32_t)-1)
		Window::MakeKeyWindow(pid, last_wid);
	
	NSEventApplicationFocus* e = NSEventApplicationFocus::Create(active_app->pid, true);
	SendActiveEvent(e);
	delete e;
	
	Desktop::UpdateMenu();
}

Application::App* Application::GetActiveApplication() {
	return active_app;
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
	app->menu = NULL;
	applications.push_back(app);
	
	Desktop::RegisterApplication(app);
	
	SetActiveApplication(pid);
	
	// Send all settings info to the application
	NSEventSettingsPixelScalingFactor* psf_event =
		NSEventSettingsPixelScalingFactor::Create(Desktop::GetPixelScalingFactor());
	SendActiveEvent(psf_event);
	delete psf_event;
	
	NSEventInitResp* resp_event = NSEventInitResp::Create();
	SendActiveEvent(resp_event);
	delete resp_event;
}

void Application::UnregisterApplication(uint32_t pid) {
	if (active_app && active_app->pid == pid) {
		SetActiveApplication(Window::FindLastVisiblePid(pid));
	}
	
	Window::DestroyWindows(pid);
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
	if (active_app && active_app->pid == pid)
		return SendActiveEvent(event);
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
	NSEventSettingsPixelScalingFactor* event = NSEventSettingsPixelScalingFactor::Create(psf);
	
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

