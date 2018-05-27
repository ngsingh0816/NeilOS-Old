//
//  Event.cpp
//  product
//
//  Created by Neil Singh on 5/24/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "Event.h"

#include "Application.h"
#include "NSEventDefs.cpp"
#include "Window.h"

#include <NeilOS/NeilOS.h>

#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <unistd.h>

namespace Event {
	uint32_t message_fd = -1;
	pthread_t message_thread = NULL;
	
	void* MessageFunc(void*);
	
	void InitEvent(uint32_t* data, uint32_t length);
	
	void SendEvent(NSEvent* event, uint32_t pid);
}

void* Event::MessageFunc(void*) {
	uint8_t* data = new uint8_t[4096];
	uint32_t priority = 0;
	uint32_t length = 0;
	while ((length = mq_receive(message_fd, reinterpret_cast<char*>(data), 4096, &priority)) != 0) {
		if (length == (uint32_t)-1)
			continue;
		
		uint8_t* temp = new uint8_t[length];
		memcpy(temp, data, length);
		NSHandler([temp, length](NSThread*) {
			if (length < sizeof(uint32_t) * 2)
				return;
			
			uint32_t* buffer = reinterpret_cast<uint32_t*>(temp);
			uint32_t event_code = buffer[0];
			
			switch (event_code) {
				case 1:
					// NSWindow event
					Window::ProcessEvent(temp, length);
					break;
				case 2:
					// Mouse event
					break;
				case 3:
					// Key event
					break;
				case 4:
					// Init event
					InitEvent(buffer, length);
					break;
				default:
					break;
			}
						
			delete[] temp;
		}).Post(NSThread::MainThread(), 0);
	}
	delete[] data;
	
	return NULL;
}

void Event::Setup() {
	message_fd = mq_open("/1", O_RDWR | O_CREAT);
	pthread_create(&message_thread, NULL, MessageFunc, NULL);
}


void Event::InitEvent(uint32_t* data, uint32_t length) {
	uint32_t sub_code = data[1];
	if (sub_code != INIT_EVENT_INIT)
		return;
	
	NSEventInit* event = NSEventInit::FromData(reinterpret_cast<uint8_t*>(data), length);
	if (!event)
		return;
	
	Application::RegisterApplication(event->GetPid(), event->GetName(), event->GetPath());

	delete event;
}
