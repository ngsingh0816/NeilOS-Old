//
//  NSApplication.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSApplication.h"

#include "NSEvent.h"
#include "Events/NSEventDefs.cpp"
#include "NSHandler.h"
#include "NSThread.h"
#include "../GUI/NSWindow.h"
#include "../GUI/NSView.h"

#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <string.h>

#include <string>

namespace NSApplication {
	NSApplicationDelegate* delegate = NULL;
	
	std::vector<NSApplicationObserver*> observers;
	
	NSMenu* menu = NULL;
	NSMenu* GetMenuPtr() {
		return menu;
	}
	
	pthread_t message_thread;
	uint32_t message_fd;
	uint32_t send_fd;
	
	float pixel_scaling_factor = 1.0f;
	void SetPixelScalingFactor(float factor);
	
	void OpenDelegateFile(std::string filename);
	
	void* MessageFunc(void*) {
		uint8_t* data = new uint8_t[32 * 1024];
		uint32_t priority = 0;
		uint32_t length = 0;
		while ((length = mq_receive(message_fd, reinterpret_cast<char*>(data), 32 * 1024, &priority)) != 0) {
			if (length == (uint32_t)-1)
				continue;
			
			uint8_t* temp = new uint8_t[length];
			memcpy(temp, data, length);
			NSHandler([temp, length](NSThread*) {
				NSEvent* event = NSEvent::FromData(temp, length);
				if (event) {
					bool process = true;
					if (delegate)
						process = delegate->ReceivedEvent(event, temp, length);
					if (process)
						event->Process();
					delete event;
				} else if (delegate)
					delegate->ReceivedMessage(temp, length);
				delete[] temp;
			}).Post(NSThread::MainThread(), 0);
		}
		delete[] data;
		
		return NULL;
	}
	
	std::vector<NSWindow*> windows;
	
	void RegisterWindow(NSWindow* window) {
		windows.push_back(window);
	}
	
	void DeregisterWindow(NSWindow* window) {
		for (unsigned int z = 0; z < windows.size(); z++) {
			if (windows[z] == window) {
				windows.erase(windows.begin() + z);
				z--;
			}
		}
	}
	
	NSWindow* GetWindow(uint32_t window_id) {
		for (unsigned int z = 0; z < windows.size(); z++) {
			if (windows[z]->GetWindowID() == window_id)
				return windows[z];
		}
		return NULL;
	}
};

// Pretty much initializes this library
void NSApplication::Setup(int argc, const char** argv) {
	// TODO:
	/*
	 * Close open files
	 * Change working directory
	 */
	NSThread::Setup();
	
	// Setup message thread
	std::string str = std::to_string(getpid());
	message_fd = mq_open(("/" + str).c_str(), O_RDWR | O_CREAT);
	pthread_create(&message_thread, NULL, MessageFunc, NULL);
	
	// Setup sending message queue
	send_fd = mq_open("/1", O_WRONLY);
	
	std::string name = "Application";
	std::string path;
	if (argc > 0) {
		path = argv[0];
		name = path.substr(path.find_last_of("/") + 1);
	}
	
	// Setup menu
	menu = new NSMenu;
	NSMenuItem* item = new NSMenuItem(name);
	NSMenuItem* sitem = new NSMenuItem("Quit");
	sitem->SetKeyEquivalent("Q", NSModifierControl);
	sitem->SetAction([](NSMenuItem*) {
		NSApplication::Exit();
	});
	item->SetSubmenu(new NSMenu);
	item->GetSubmenu()->AddItem(sitem);
	menu->AddItem(item);
	
	// Setup random numbers
	srand(time(NULL));
}
	
// Default main function for all GUI applications
int NSApplication::Run(int argc, const char** argv, NSApplicationDelegate* d) {
	Setup(argc, argv);
	delegate = d;
	
	NSEventInitResp::SetCallback([]() {
		if (delegate)
			delegate->DidFinishLaunching();
	});
	
	std::string name;
	std::string path;
	if (argc > 0) {
		path = argv[0];
		name = path.substr(path.find_last_of("/") + 1);
	}
	NSEventInit* event = NSEventInit::Create(getpid(), name, path);
	SendEvent(event);
	delete event;
	
	SetMenu(menu);
	
	NSRunLoop* loop = NSThread::MainThread()->GetRunLoop();
	loop->Run();
	
	NSEventApplicationQuit* quit = NSEventApplicationQuit::Create(getpid());
	SendEvent(quit);
	delete quit;
	
	// Delete menu
	delete menu;
	
	// Delete message queue
	mq_close(message_fd);
	std::string str = std::to_string(getpid());
	mq_unlink(str.c_str());
	
	// Close server queue
	mq_close(send_fd);
	
	return 0;
}

// Exit application
void NSApplication::Exit() {
	if (delegate)
		delegate->WillExit();
	NSThread::MainThread()->GetRunLoop()->Stop();
}
	
// Application callbacks
void NSApplication::SetDelegate(NSApplicationDelegate* d) {
	delegate = d;
}

NSApplicationDelegate* NSApplication::GetDelegate() {
	return delegate;
}

void NSApplication::AddObserver(NSApplicationObserver* observer) {
	observers.push_back(observer);
}

void NSApplication::RemoveObserver(NSApplicationObserver* observer) {
	for (unsigned int z = 0; z < observers.size(); z++) {
		if (observers[z] == observer) {
			observers.erase(observers.begin() + z);
			break;
		}
	}
}

// Send an event to the server
bool NSApplication::SendEvent(NSEvent* event) {
	uint32_t len = 0;
	uint8_t* bytes = event->Serialize(&len);
	if (!bytes)
		return false;
	mq_send(send_fd, reinterpret_cast<const char*>(bytes), len, MQ_PRIO_MAX/2);
	delete[] bytes;
	return true;
}

// Menu
void NSApplication::SetMenu(NSMenu* m) {
	if (!m)
		return;
	
	if (menu != m && menu)
		delete menu;
	
	menu = m;
	
	NSEventApplicationSetMenu* e = NSEventApplicationSetMenu::Create(getpid(), menu);
	SendEvent(e);
	delete e;
}

NSMenu NSApplication::GetMenu() {
	return *menu;
}

// Windows
NSWindow* NSApplication::GetKeyWindow() {
	for (auto window : windows) {
		if (window->IsKeyWindow())
			return window;
	}
	return NULL;
}

// Open application
bool NSApplication::OpenApplication(std::string path, ...) {
	// TODO: args
	int32_t f = fork();
	if (f == 0) {
		char* argv[] = { (char*)path.c_str() };
		if (execv(path.c_str(), argv) != 0) {
			exit(-1);
			return false;
		}
		return true;
	}
	else if (f == -1)
		return false;
	
	return true;
}

bool NSApplication::OpenFile(std::string path) {
	// TODO: implement
	return false;
}

// Pixel Scaling Factor
float NSApplication::GetPixelScalingFactor() {
	return pixel_scaling_factor;
}

void NSApplication::SetPixelScalingFactor(float factor) {
	pixel_scaling_factor = factor;
	
	for (auto window : windows) {
		if (!window->IsVisible())
			continue;
		NSView* view = window->GetContentView();
		view->SetNeedsDisplay();
	}
}

void NSApplication::SetFocus(bool focus) {
	for (auto& i : observers) {
		if (focus)
			i->GainFocus();
		else
			i->LoseFocus();
	}
}

void NSApplication::OpenDelegateFile(std::string filename) {
	if (delegate) {
		if (delegate->OpenFile(filename))
			return;
	}
	
	// TODO: indiciate failure with an alert?
}
