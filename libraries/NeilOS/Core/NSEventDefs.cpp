//
//  NSEventDefs.cpp
//  product
//
//  Created by Neil Singh on 5/24/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "NeilOS/NeilOS.h"

#include <string.h>

#include <string>

#define EVENT_WINDOW_ID		1
#define EVENT_MOUSE_ID		2
#define EVENT_KEY_ID		3
#define EVENT_INIT_ID		4
#define EVENT_SETTINGS_ID	5

#define copy(x, len) { memcpy(&buffer[pos], x, (len)); pos += (len); }
#define copy_rect(x) { uint8_t* temp = x.Serialize(NULL); memcpy(&buffer[pos], temp, sizeof(float)*4);\
						pos += sizeof(float) * 4; delete[] temp; }

#define WINDOW_EVENT_CREATE_ID			0
#define WINDOW_EVENT_DESTROY_ID			1
#define WINDOW_EVENT_SHOW_ID			2
#define WINDOW_EVENT_MAKE_KEY_ID		3
#define WINDOW_EVENT_SET_TITLE_ID		4
#define WINDOW_EVENT_SET_FRAME_ID		5
#define WINDOW_EVENT_DRAW_ID			6

#define INIT_EVENT_INIT		1
#define INIT_EVENT_RESP		2

#define SETTINGS_EVENT_PSF	1

using std::string;

class NSEventInit : public NSEvent {
public:
	static NSEventInit* Create(uint32_t pid, std::string name, std::string path) {
		return new NSEventInit(pid, name, path);
	}
	
	static NSEventInit* FromData(uint8_t* data, uint32_t length) {
		if (length < sizeof(uint32_t) * 2)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_INIT_ID || buffer[1] != INIT_EVENT_INIT)
			return NULL;
		
		uint32_t pid = buffer[2];
		
		uint32_t pos = 3;
		uint32_t name_len = buffer[pos++];
		char name_buf[name_len + 1];
		memcpy(name_buf, &buffer[pos], name_len);
		name_buf[name_len] = 0;
		pos += name_len;
		
		uint32_t path_len = buffer[pos++];
		char path_buf[path_len + 1];
		memcpy(path_buf, &buffer[pos], path_len);
		path_buf[path_len] = 0;
		pos += path_len;
		
		return Create(pid, name_buf, path_buf);
	}
	
	uint32_t GetPid() const {
		return process_id;
	}
	
	void SetPid(uint32_t p) {
		process_id = p;
	}
	
	std::string GetName() const {
		return name;
	}
	
	void SetName(std::string n) {
		name = n;
	}
	
	std::string GetPath() const {
		return path;
	}
	
	void SetPath(std::string p) {
		path = p;
	}
	
	void Process() override {
	}
	
	uint8_t* Serialize(uint32_t* length_out) const {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// name.length()
		total_length += name.length();
		total_length += sizeof(uint32_t);	// path.length()
		total_length += path.length();
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_INIT_ID;
		uint32_t event_sub_id = INIT_EVENT_INIT;
		uint32_t name_length = name.length();
		uint32_t path_length = path.length();
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&process_id, sizeof(uint32_t));
		copy(&name_length, sizeof(uint32_t));
		copy(name.c_str(), name_length);
		copy(&path_length, sizeof(uint32_t));
		copy(path.c_str(), path_length);
		
		*length_out = total_length;
		return buffer;
	}
	
private:
	NSEventInit(uint32_t p, std::string n, std::string pa) {
		process_id = p;
		name = n;
		path = pa;
	}
	
	uint32_t process_id;
	std::string name;
	std::string path;
};

class NSEventInitResp : public NSEvent {
public:
	static std::function<void()> function;
	
	static NSEventInitResp* Create() {
		return new NSEventInitResp();
	}
	
	static NSEventInitResp* FromData(uint8_t* data, uint32_t length) {
		if (length != sizeof(uint32_t) * 2)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] == EVENT_INIT_ID && buffer[1] == INIT_EVENT_RESP)
			return Create();
		return NULL;
	}
	
	static void SetCallback(std::function<void()> func) {
		function = func;
	}
	
	void Process() override {
		if (function)
			function();
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_INIT_ID;
		uint32_t event_sub_id = INIT_EVENT_RESP;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		
		*length_out = total_length;
		return buffer;
	}
	
private:
	NSEventInitResp() {
	}
};

namespace NSApplication {
	void SetPixelScalingFactor(float factor);
}

class NSEventPixelScalingFactor : public NSEvent {
public:
	static NSEventPixelScalingFactor* Create(float psf) {
		return new NSEventPixelScalingFactor(psf);
	}
	
	float GetPixelScalingFactor() const {
		return pixel_scaling_factor;
	}
	
	void SetPixelScalingFactor(float psf) {
		pixel_scaling_factor = psf;
	}
	
	void Process() override {
		NSApplication::SetPixelScalingFactor(pixel_scaling_factor);
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(float);	// psf
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_SETTINGS_ID;
		uint32_t event_sub_id = SETTINGS_EVENT_PSF;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pixel_scaling_factor, sizeof(float));
		
		*length_out = total_length;
		return buffer;
	}
	
private:
	NSEventPixelScalingFactor(float psf) {
		pixel_scaling_factor = psf;
	}
	
	float pixel_scaling_factor;
};

// Window events
class NSEventWindowCreate : public NSEvent {
public:
	NSEventWindowCreate(string t, NSRect f, uint32_t p, uint32_t win_id, graphics_context_t* c) {
		title = t;
		frame = f;
		pid = p;
		window_id = win_id;
		context = *c;
	}
	
	static NSEventWindowCreate* FromData(uint8_t* data, uint32_t length) {
		if (length <= sizeof(uint32_t) * 2)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_WINDOW_ID || buffer[1] != WINDOW_EVENT_CREATE_ID)
			return NULL;
		
		uint32_t title_len = buffer[2];
		char buf[title_len + 1];
		memcpy(buf, &buffer[3], title_len);
		buf[title_len] = 0;
		
		uint32_t pos = sizeof(uint32_t) * 3 + title_len;
		string title = string(buf);
		NSRect frame = NSRect::FromData(&data[pos], sizeof(float) * 4);
		pos += sizeof(float) * 4;
		uint32_t pid, window_id;
		memcpy(&pid, &data[pos], sizeof(uint32_t));
		pos += sizeof(uint32_t);
		memcpy(&window_id, &data[pos], sizeof(uint32_t));
		pos += sizeof(uint32_t);
		graphics_context_t context;
		memcpy(&context, &data[pos], sizeof(graphics_context_t));
		
		return new NSEventWindowCreate(title, frame, pid, window_id, &context);
	}

	string GetTitle() const {
		return title;
	}
	
	void SetTitle(string t) {
		title = t;
	}
	
	NSRect GetFrame() const {
		return frame;
	}
	
	void SetFrame(NSRect rect) {
		frame = rect;
	}
	
	uint32_t GetPid() const {
		return pid;
	}
	
	void SetPid(uint32_t p) {
		pid = p;
	}
	
	uint32_t GetWindowID() const {
		return window_id;
	}
	
	void SetWindowID(uint32_t id) {
		window_id = id;
	}
	
	graphics_context_t GetGraphicsContext() const {
		return context;
	}
	
	void SetGraphicsContext(graphics_context_t* c) {
		context = *c;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// title.length()
		total_length += title.length();
		total_length += sizeof(float) * 4;		// frame
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(graphics_context_t);
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_WINDOW_ID;
		uint32_t event_sub_id = WINDOW_EVENT_CREATE_ID;
		uint32_t title_len = title.length();
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&title_len, sizeof(uint32_t));
		copy(title.c_str(), title_len);
		copy_rect(frame);
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		copy(&context, sizeof(graphics_context_t));
		
		*length_out = total_length;
		return buffer;
	}
private:
	string title;
	NSRect frame;
	uint32_t pid;
	uint32_t window_id;
	graphics_context_t context;
};

class NSEventWindowDestroy : public NSEvent {
public:
	NSEventWindowDestroy(uint32_t p, uint32_t id) {
		pid = p;
		window_id = id;
	}
	
	static NSEventWindowDestroy* FromData(uint8_t* data, uint32_t length) {
		if (length != sizeof(uint32_t) * 4)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_WINDOW_ID || buffer[1] != WINDOW_EVENT_DESTROY_ID)
			return NULL;
		
		uint32_t pid = buffer[2], window_id = buffer[3];
		return new NSEventWindowDestroy(pid, window_id);
	}
	
	uint32_t GetPid() const {
		return pid;
	}
	
	void SetPid(uint32_t p) {
		pid = p;
	}
	
	uint32_t GetWindowID() const {
		return window_id;
	}
	
	void SetWindowID(uint32_t id) {
		window_id = id;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_WINDOW_ID;
		uint32_t event_sub_id = WINDOW_EVENT_DESTROY_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
};

class NSEventWindowShow : public NSEvent {
public:
	NSEventWindowShow(uint32_t p, uint32_t id, bool s) {
		pid = p;
		window_id = id;
		show = s;
	}
	
	static NSEventWindowShow* FromData(uint8_t* data, uint32_t length) {
		if (length != sizeof(uint32_t) * 4 + sizeof(bool))
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_WINDOW_ID || buffer[1] != WINDOW_EVENT_SHOW_ID)
			return NULL;
		
		uint32_t pid = buffer[2], window_id = buffer[3];
		bool show = data[sizeof(uint32_t) * 4];
		
		return new NSEventWindowShow(pid, window_id, show);
	}
	
	uint32_t GetPid() const {
		return pid;
	}
	
	void SetPid(uint32_t p) {
		pid = p;
	}
	
	uint32_t GetWindowID() const {
		return window_id;
	}
	
	void SetWindowID(uint32_t id) {
		window_id = id;
	}
	
	bool GetShow() const {
		return show;
	}
	
	void SetShow(bool s) {
		show = s;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(bool);		// show;
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_WINDOW_ID;
		uint32_t event_sub_id = WINDOW_EVENT_SHOW_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		copy(&show, sizeof(bool));
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
	bool show;
};

class NSEventWindowMakeKey : public NSEvent {
public:
	NSEventWindowMakeKey(uint32_t p, uint32_t id) {
		pid = p;
		window_id = id;
	}
	
	static NSEventWindowMakeKey* FromData(uint8_t* data, uint32_t length) {
		if (length != sizeof(uint32_t) * 4)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_WINDOW_ID || buffer[1] != WINDOW_EVENT_MAKE_KEY_ID)
			return NULL;
		
		uint32_t pid = buffer[2], window_id = buffer[3];
		return new NSEventWindowMakeKey(pid, window_id);
	}
	
	uint32_t GetPid() const {
		return pid;
	}
	
	void SetPid(uint32_t p) {
		pid = p;
	}
	
	uint32_t GetWindowID() const {
		return window_id;
	}
	
	void SetWindowID(uint32_t id) {
		window_id = id;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_WINDOW_ID;
		uint32_t event_sub_id = WINDOW_EVENT_MAKE_KEY_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
};

class NSEventWindowSetTitle : public NSEvent {
public:
	NSEventWindowSetTitle(uint32_t p, uint32_t id, string t) {
		pid = p;
		window_id = id;
		title = t;
	}
	
	static NSEventWindowSetTitle* FromData(uint8_t* data, uint32_t length) {
		if (length < sizeof(uint32_t) * 5)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_WINDOW_ID || buffer[1] != WINDOW_EVENT_SET_TITLE_ID)
			return NULL;
		
		uint32_t pid = buffer[2], window_id = buffer[3];
		uint32_t title_len = buffer[4];
		char buf[title_len + 1];
		memcpy(buf, &data[sizeof(uint32_t) * 5], title_len);
		buf[title_len] = 0;
		
		return new NSEventWindowSetTitle(pid, window_id, string(buf));
	}
	
	uint32_t GetPid() const {
		return pid;
	}
	
	void SetPid(uint32_t p) {
		pid = p;
	}
	
	uint32_t GetWindowID() const {
		return window_id;
	}
	
	void SetWindowID(uint32_t id) {
		window_id = id;
	}
	
	string GetTitle() const {
		return title;
	}
	
	void SetTitle(string t) {
		title = t;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(uint32_t);	// title length
		total_length += title.length();
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_WINDOW_ID;
		uint32_t event_sub_id = WINDOW_EVENT_SET_TITLE_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		uint32_t title_len = title.length();
		copy(&title_len, sizeof(uint32_t));
		copy(title.c_str(), title_len);
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
	string title;
};

class NSEventWindowSetFrame : public NSEvent {
public:
	NSEventWindowSetFrame(uint32_t p, uint32_t id, NSRect f) {
		pid = p;
		window_id = id;
		frame = f;
	}
	
	static NSEventWindowSetFrame* FromData(uint8_t* data, uint32_t length) {
		if (length != sizeof(uint32_t) * 4 + sizeof(float) * 4)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_WINDOW_ID || buffer[1] != WINDOW_EVENT_SET_FRAME_ID)
			return NULL;
		
		uint32_t pid = buffer[2], window_id = buffer[3];
		NSRect frame = NSRect::FromData(&data[sizeof(uint32_t) * 4], sizeof(float) * 4);
		
		return new NSEventWindowSetFrame(pid, window_id, frame);
	}
	
	uint32_t GetPid() const {
		return pid;
	}
	
	void SetPid(uint32_t p) {
		pid = p;
	}
	
	uint32_t GetWindowID() const {
		return window_id;
	}
	
	void SetWindowID(uint32_t id) {
		window_id = id;
	}
	
	NSRect GetFrame() const {
		return frame;
	}
	
	void SetFrame(NSRect f) {
		frame =f ;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(float) * 4;		// frame
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_WINDOW_ID;
		uint32_t event_sub_id = WINDOW_EVENT_SET_FRAME_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&window_id, sizeof(uint32_t));
		copy_rect(frame);
		
		*length_out = total_length;
		return buffer;
	}
private:
	uint32_t pid;
	uint32_t window_id;
	NSRect frame;
};

class NSEventWindowDraw : public NSEvent {
public:
	static NSEventWindowDraw* Create(uint32_t pid, uint32_t wid, std::vector<NSRect> rects) {
		return new NSEventWindowDraw(pid, wid, rects);
	}
	
	static NSEventWindowDraw* FromData(uint8_t* data, uint32_t length) {
		if (length < sizeof(uint32_t) * 5)
			return NULL;
		uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
		if (buffer[0] != EVENT_WINDOW_ID || buffer[1] != WINDOW_EVENT_DRAW_ID)
			return NULL;
		
		uint32_t pid = buffer[2], window_id = buffer[3], rect_size = buffer[4];
		std::vector<NSRect> rects;
		rects.reserve(rect_size);
		uint32_t pos = sizeof(uint32_t) * 5;
		for (unsigned int z = 0; z < rect_size; z++) {
			rects.emplace_back(NSRect::FromData(&data[pos], sizeof(float) * 4));
			pos += sizeof(float) * 4;
		}
		
		return Create(pid, window_id, rects);
	}
	
	uint32_t GetPid() const {
		return pid;
	}
	
	void SetPid(uint32_t p) {
		pid = p;
	}
	
	uint32_t GetWindowID() const {
		return wid;
	}
	
	void SetWindowID(uint32_t w) {
		wid = w;
	}
	
	std::vector<NSRect> GetUpdateRects() const {
		return update_rects;
	}
	
	void SetUpdateRects(std::vector<NSRect> rects) {
		update_rects = rects;
	}
	
	uint8_t* Serialize(uint32_t* length_out) const override {
		uint32_t total_length = 0;
		total_length += sizeof(uint32_t) * 2;// event id's
		total_length += sizeof(uint32_t);	// pid
		total_length += sizeof(uint32_t);	// window_id
		total_length += sizeof(uint32_t);	// update_rects.size()
		total_length += sizeof(float) * 4 * update_rects.size();
		uint8_t* buffer = new uint8_t[total_length];
		uint32_t pos = 0;
		
		uint32_t event_id = EVENT_WINDOW_ID;
		uint32_t event_sub_id = WINDOW_EVENT_DRAW_ID;
		copy(&event_id, sizeof(uint32_t));
		copy(&event_sub_id, sizeof(uint32_t));
		copy(&pid, sizeof(uint32_t));
		copy(&wid, sizeof(uint32_t));
		uint32_t num_rects = update_rects.size();
		copy(&num_rects, sizeof(uint32_t));
		for (unsigned int z = 0; z < update_rects.size(); z++)
			copy_rect(update_rects[z]);
		
		*length_out = total_length;
		return buffer;
	}
	
private:
	NSEventWindowDraw(uint32_t p, uint32_t w, std::vector<NSRect> rects) {
		pid = p;
		wid = w;
		update_rects = rects;
	}
	
	uint32_t pid;
	uint32_t wid;
	std::vector<NSRect> update_rects;
};

#undef copy
