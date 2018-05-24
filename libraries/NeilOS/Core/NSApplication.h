//
//  NSApplication.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSAPPLICATION_H
#define NSAPPLICATION_H

#include <stdint.h>
#include <string>

class NSEvent;

class NSApplicationDelegate {
public:
	NSApplicationDelegate() {}
	virtual ~NSApplicationDelegate() {}
	
	// Initialization
	virtual void DidFinishLaunching() {};
	virtual void WillExit() {};
	
	// Event handling
	virtual bool ReceivedEvent(NSEvent* event) { return true; };	// return true to process, false to ignore
	virtual void ReceivedMessage(uint8_t* data, unsigned int length) {};
	
	// Events
	// OpenFile(), etc
private:
};

namespace NSApplication {
	// Pretty much initializes this library
	void Setup(int argc, const char** argv);
	// Default main function for all GUI applications
	int Run(int argc, const char** argv, NSApplicationDelegate* delegate);
	
	// Exit application
	void Exit();
	
	// Application callbacks
	void SetDelegate(NSApplicationDelegate* delegate);
	NSApplicationDelegate* GetDelegate();
	
	// Send an event to the server
	bool SendEvent(NSEvent* event);
	
	// Open application
	bool OpenApplication(std::string path, ...);
};

#endif /* NSAPPLICATION_H */
