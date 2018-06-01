//
//  NSApplication.h
//  product
//
//  Created by Neil Singh on 2/13/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef NSAPPLICATION_H
#define NSAPPLICATION_H

#include "../GUI/NSMenu.h"
#include "../GUI/NSWindow.h"

#include <stdint.h>
#include <string>

class NSEvent;

class NSApplicationDelegate {
public:
	NSApplicationDelegate() {}
	virtual ~NSApplicationDelegate() {}
	
	// Initialization
	virtual void DidFinishLaunching() {}
	virtual void WillExit() {}
	
	// Event handling (return true to process, false to ignore)
	virtual bool ReceivedEvent(NSEvent* event, uint8_t* data, unsigned int length) { return true; }
	virtual void ReceivedMessage(uint8_t* data, unsigned int length) {}
	
	// Events
	virtual bool OpenFile(std::string filename) { return false; }
	virtual bool ShouldExit() { return true; }
private:
};

// For events that you can only observe
class NSApplicationObserver {
public:
	virtual void GainFocus() {}
	virtual void LoseFocus() {}
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
	
	// Application observers
	void AddObserver(NSApplicationObserver* observer);
	void RemoveObserver(NSApplicationObserver* observer);
	
	// Send an event to the server
	bool SendEvent(NSEvent* event);
	
	// Menu
	void SetMenu(NSMenu* menu);
	NSMenu GetMenu();
	
	// Windows
	NSWindow* GetKeyWindow();
	
	// Open
	bool OpenApplication(std::string path, ...);
	bool OpenFile(std::string path);
	
	// Pixel Scaling Factor
	float GetPixelScalingFactor();
};

#endif /* NSAPPLICATION_H */
