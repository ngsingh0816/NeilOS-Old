//
//  NSApplication.cpp
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NSApplication.h"

#include "NSEvent.h"
#include "NSHandler.h"
#include "NSThread.h"

#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>

#include <string>

namespace NSApplication {
	NSApplicationDelegate* delegate = NULL;
	pthread_t message_thread;
	uint32_t message_fd;
	uint32_t send_fd;
	
	void* MessageFunc(void*) {
		uint8_t* data = new uint8_t[4096];
		uint32_t priority = 0;
		uint32_t length = 0;
		while ((length = mq_receive(message_fd, reinterpret_cast<char*>(data), 4096, &priority)) != 0) {
			if (length == (uint32_t)-1)
				continue;
			
			NSEvent* event = NSEvent::FromData(data, length);
			if (event) {
				bool process = true;
				if (delegate)
					process = delegate->ReceivedEvent(event);
				if (process) {
					NSHandler([event](NSThread*) {
						event->Process();
						delete event;
					}).Post(event->GetPriority());
				} else
					delete event;
			} else if (delegate)
				delegate->ReceivedMessage(data, length);
		}
		delete data;
		
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
}
	
// Default main function for all GUI applications
int NSApplication::Run(int argc, const char** argv, NSApplicationDelegate* d) {
	Setup(argc, argv);
	delegate = d;
	NSRunLoop* loop = NSThread::MainThread()->GetRunLoop();
	loop->AddEvent(NSEventFunction::Create([]() {
		if (delegate)
			delegate->DidFinishLaunching();
	}));
	loop->Run();
	
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

// Open application
bool NSApplication::OpenApplication(std::string path, ...) {
	// TODO: args
	int32_t f = fork();
	if (f == 0) {
		if (execv(path.c_str(), NULL) != 0) {
			exit(-1);
			return false;
		}
		return true;
	}
	else if (f == -1)
		return false;
	
	return true;
}
