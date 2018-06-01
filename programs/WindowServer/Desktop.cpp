//
//  Desktop.cpp
//  product
//
//  Created by Neil Singh on 2/16/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "Desktop.h"

#include "Application.h"
#include "Window.h"

#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <vector>

#define BORDER_RECT	NSRect(-2, -2, 4, 4)

using std::vector;

graphics_info_t graphics_info;
graphics_context_t context;

namespace Desktop {
	bool loaded = false;
	
	NSImage* background_image = NULL;
	NSImage* cursor_image = NULL;
	NSSound* intro_sound = NULL;
	
	NSMenu* app_menu;		// on the left
	NSMenu* system_menu;	// on the right
	NSMenu* task_bar;		// on the bottom
	const float menu_height = 30;
	
	uint32_t square_vbo;
	uint32_t color_vbo;
	uint32_t fade_vbo;
	uint32_t uv_vbo;
	uint32_t background_vbo;
	graphics_vertex_array_t background_vao[3];
	graphics_vertex_array_t fade_vao[2];
	
	pthread_t mouse_thread;
	volatile bool mouse_down = false;
	bool mouse_desktop_down = false;
	bool mouse_menu_down = false;
	NSPoint mouse_down_pos;
	NSPoint mouse_pos;
	uint32_t mouse_vbo;
	graphics_vertex_array_t mouse_vao[2];
	
	pthread_t key_thread;
	NSModifierFlags modifier_flags = NSModifierNone;
	bool caps_lock_activated = false;
	bool caps_lock_down = false;
	
	vector<NSRect> dirty_rects;
	NSLock rect_lock;
	bool update_requested = false;
	float pixel_scaling_factor = 2.0f;
	
	struct timeval fade_start;
	
	const float square[] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	const float square_colors[] = {
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
	};
	float fade_colors[] = {
		0, 0, 0, 1.0,
		0, 0, 0, 1.0,
		0, 0, 0, 1.0,
		0, 0, 0, 1.0,
	};
	const float mouse_colors[] = {
		0.5, 0.5, 0.5, 0.5,
		0.5, 0.5, 0.5, 0.5,
		0.5, 0.5, 0.5, 0.5,
		0.5, 0.5, 0.5, 0.5,
	};
	const float uv[] = {
		0, 0,
		1.0, 0,
		0.0, 1.0,
		1.0, 1.0
	};
	
	void* MouseThread(void*);
	void* KeyThread(void*);
	void FadeFunc(NSTimer*);
	
	bool MouseMenu(NSPoint p, NSMouseType type, NSMouseButton mouse, bool (NSMenu::*func)(NSEventMouse*));
	bool KeyMenu(unsigned char key, NSModifierFlags flags, bool down, bool (NSMenu::*func)(NSEventKey*));
	
	std::string GetTime();
	uint32_t SecondsToNextMinute();
}

float Desktop::GetPixelScalingFactor() {
	return pixel_scaling_factor;
}

void Desktop::SetPixelScalingFactor(float factor) {
	float old_factor = pixel_scaling_factor;
	pixel_scaling_factor = factor;
	
	graphics_info.resolution_x = round(graphics_info.resolution_x * old_factor / factor);
	graphics_info.resolution_y = round(graphics_info.resolution_y * old_factor / factor);
	
	NSMatrix perspective = NSMatrix::Ortho2D(0, graphics_info.resolution_x, graphics_info.resolution_y, 0);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_PROJECTION, perspective);
		
	Application::SendPixelScalingFactorEvent(factor);
	
	// Update menus
	NSRect system_menu_rect = NSRect(NSPoint(graphics_info.resolution_x, 0),
									 NSSize(system_menu->GetSize().width, menu_height - 1));	// Account for border
	system_menu_rect.origin.x -= system_menu_rect.size.width;
	system_menu->SetFrame(system_menu_rect);
	NSRect app_menu_rect = NSRect(NSPoint(), NSSize(system_menu_rect.origin.x, menu_height - 1));
	app_menu->SetFrame(app_menu_rect);
	task_bar->SetFrame(NSRect(0, graphics_info.resolution_y - menu_height + 1,
							  graphics_info.resolution_x, menu_height - 1));
	
	UpdateRect(NSRect(0, 0, graphics_info.resolution_x, graphics_info.resolution_y));
}

std::string Desktop::GetTime() {
	char buf[20];
	time_t t = time(NULL);
	strftime(buf, sizeof(buf), "%a %I:%M %p", localtime(&t));
	return std::string(buf);
}

uint32_t Desktop::SecondsToNextMinute() {
	char buf[20];
	time_t t = time(NULL);
	strftime(buf, sizeof(buf), "%S", localtime(&t));
	return (60 - std::stoi(buf)) + 1;
}

void Desktop::Load(volatile float* p, float percent_start, float percent_end) {
#define percent(x) *p = percent_start + (percent_end - percent_start) / 100.0f * x
	intro_sound = NSSound::Create("/system/sounds/boot.wav");
	percent(10);
	
	cursor_image = new NSImage("/system/images/cursors/default.png");
	cursor_image->SetSize(cursor_image->GetSize() * Desktop::GetPixelScalingFactor());
	percent(20);
	
	background_image = new NSImage("/system/images/background.jpg");
	int img_width = int(background_image->GetSize().width + 0.5f);
	int img_height = int(background_image->GetSize().height + 0.5f);
	
	percent(50);
	
	square_vbo = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(square_vbo, square, sizeof(square));
	color_vbo = graphics_buffer_create(sizeof(square_colors), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(color_vbo, square_colors, sizeof(square_colors));
	fade_vbo = graphics_buffer_create(sizeof(fade_colors), GRAPHICS_BUFFER_DYNAMIC);
	graphics_buffer_data(fade_vbo, fade_colors, sizeof(fade_colors));
	uv_vbo = graphics_buffer_create(sizeof(uv), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(uv_vbo, uv, sizeof(uv));
	mouse_vbo = graphics_buffer_create(sizeof(mouse_colors), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(mouse_vbo, mouse_colors, sizeof(mouse_colors));
	
	percent(60);
	
	background_vbo = graphics_buffer_create(img_width, img_height, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(background_vbo, background_image->GetPixelData(),
						 img_width, img_height, img_width * img_height * 4);
	
	percent(70);
	
	memset(background_vao, 0, sizeof(graphics_vertex_array_t) * 3);
	background_vao[0].bid = square_vbo;
	background_vao[0].offset = 0;
	background_vao[0].stride = 2 * sizeof(float);
	background_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	background_vao[0].usage = GRAPHICS_USAGE_POSITION;
	background_vao[1].bid = color_vbo;
	background_vao[1].offset = 0;
	background_vao[1].stride = 4 * sizeof(float);
	background_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	background_vao[1].usage = GRAPHICS_USAGE_COLOR;
	background_vao[2].bid = uv_vbo;
	background_vao[2].offset = 0;
	background_vao[2].stride = 2 * sizeof(float);
	background_vao[2].type = GRAPHICS_TYPE_FLOAT2;
	background_vao[2].usage = GRAPHICS_USAGE_TEXCOORD;
	
	memset(fade_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	fade_vao[0].bid = square_vbo;
	fade_vao[0].offset = 0;
	fade_vao[0].stride = 2 * sizeof(float);
	fade_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	fade_vao[0].usage = GRAPHICS_USAGE_POSITION;
	fade_vao[1].bid = fade_vbo;
	fade_vao[1].offset = 0;
	fade_vao[1].stride = 4 * sizeof(float);
	fade_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	fade_vao[1].usage = GRAPHICS_USAGE_COLOR;
	
	memset(mouse_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	mouse_vao[0].bid = square_vbo;
	mouse_vao[0].offset = 0;
	mouse_vao[0].stride = 2 * sizeof(float);
	mouse_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	mouse_vao[0].usage = GRAPHICS_USAGE_POSITION;
	mouse_vao[1].bid = mouse_vbo;
	mouse_vao[1].offset = 0;
	mouse_vao[1].stride = 4 * sizeof(float);
	mouse_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	mouse_vao[1].usage = GRAPHICS_USAGE_COLOR;
		
	pthread_create(&mouse_thread, NULL, MouseThread, NULL);
	pthread_create(&key_thread, NULL, KeyThread, NULL);
	
	percent(80);
	
	app_menu = new NSMenu;
	system_menu = new NSMenu;
	task_bar = new NSMenu;
	
	// TODO: temp
	NSMenuItem* item = new NSMenuItem("File");
	item->SetSubmenu(new NSMenu);
	item->GetSubmenu()->AddItem(new NSMenuItem("Open"));
	item->GetSubmenu()->AddItem(new NSMenuItem("Close"));
	item->GetSubmenu()->GetItems()[1]->SetIsEnabled(false);
	item->GetSubmenu()->AddItem(new NSMenuItem("Save"));
	item->GetSubmenu()->GetItems()[2]->SetKeyEquivalent("s", NSModifierCommand);
	app_menu->AddItem(item);
	app_menu->AddItem(NSMenuItem::SeparatorItem());
	item = new NSMenuItem("Edit");
	item->SetSubmenu(new NSMenu);
	item->GetSubmenu()->AddItem(new NSMenuItem("Copy"));
	item->GetSubmenu()->AddItem(new NSMenuItem("Paste"));
	app_menu->AddItem(item);
	item = new NSMenuItem("View");
	item->SetSubmenu(new NSMenu);
	item->GetSubmenu()->AddItem(new NSMenuItem("Item 1"));
	item->GetSubmenu()->AddItem(new NSMenuItem("Item 2"));
	item->GetSubmenu()->AddItem(NSMenuItem::SeparatorItem());
	item->GetSubmenu()->AddItem(new NSMenuItem("Item 3"));
	item->GetSubmenu()->GetItems()[3]->SetSubmenu(new NSMenu);
	item->GetSubmenu()->GetItems()[3]->GetSubmenu()->AddItem(new NSMenuItem("Yay"));
	item->GetSubmenu()->AddItem(new NSMenuItem("Item 4"));
	item->GetSubmenu()->AddItem(new NSMenuItem("Item 5"));
	item->GetSubmenu()->AddItem(new NSMenuItem("Item 6"));
	app_menu->AddItem(item);
	
	system_menu->AddItem(new NSMenuItem(GetTime()));
	system_menu->AddItem(new NSMenuItem("NeilOS"));		// TODO: replace with image?
	NSRect system_menu_rect = NSRect(NSPoint(graphics_info.resolution_x, 0),
									 NSSize(system_menu->GetSize().width, menu_height - 1));	// Account for border
	system_menu_rect.origin.x -= system_menu_rect.size.width;
	system_menu->SetFrame(system_menu_rect);
	system_menu->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	system_menu->SetContext(&context);
	
	NSRect app_menu_rect = NSRect(NSPoint(), NSSize(system_menu_rect.origin.x, menu_height - 1));
	app_menu->SetFrame(app_menu_rect);
	app_menu->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	app_menu->SetContext(&context);
	
	percent(90);
	
	NSImage* start_image = new NSImage("/system/images/start.png");
	start_image->SetScaledSize(NSSize(menu_height - 5, menu_height - 5));
	item = new NSMenuItem(start_image, false);
	item->SetBorderHeight(2);
	NSMenuItem* sitem = new NSMenuItem("Calculator");
	sitem->SetFont(NSFont(30));
	sitem->SetAction([](NSMenuItem*) {
		NSApplication::OpenApplication("/Applications/Calculator");
	});
	item->SetSubmenu(new NSMenu);
	item->GetSubmenu()->AddItem(sitem);
	task_bar->AddItem(item);
	delete start_image;
	task_bar->AddItem(NSMenuItem::SeparatorItem());
	task_bar->SetOrientation(NSMenuOrientationUp);
	task_bar->SetHighlightColor(NSColor<float>::UIDarkGrayColor());
	task_bar->SetFrame(NSRect(0, graphics_info.resolution_y - menu_height + 1,
							  graphics_info.resolution_x, menu_height - 1));
	task_bar->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	task_bar->SetContext(&context);
	
	percent(98);
	
	// Create time timer
	NSTimer* timer = NSTimer::Create([](NSTimer*) {
		system_menu->GetItems()[0]->SetTitle(GetTime());
		NSTimer::Create([](NSTimer*) {
			system_menu->GetItems()[0]->SetTitle(GetTime());
		}, 60, true);
	}, SecondsToNextMinute(), false, false);
	NSThread::MainThread()->GetRunLoop()->AddTimer(timer);
	
	
	loaded = true;
	percent(100);
	
#undef percent
}

void* Desktop::MouseThread(void*) {
	int fd = open("/dev/mouse", O_RDONLY);
	for (;;) {
		uint8_t buffer[9];
		read(fd, buffer, 9);
		int32_t* b = (int32_t*)&buffer;
		
		static int32_t prevb0 = 0, prevb1 = 0;
		static bool prev_down[3] = { false, false, false };
		NSPoint p = NSPoint(b[0], b[1]);
		// left, middle, right
		bool down[3] = { buffer[8] & 0x1, (buffer[8] >> 2) & 0x1, (buffer[8] >> 1) & 0x1 };
		int scroll_x = (buffer[8] >> 3) & 0x3;
		int scroll_y = (buffer[9] >> 5) & 0x3;
		if (loaded) {
			for (int z = 0; z < 3; z++) {
				if (down[z] && !prev_down[z]) {
					NSHandler([p, z](NSThread*) {
						MouseDown(p, (NSMouseButton)((int)NSMouseButtonLeft + z));
					}).Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
				} else if (!down[z] && prev_down[z]) {
					NSHandler([p, z](NSThread*) {
						MouseUp(p, (NSMouseButton)((int)NSMouseButtonLeft + z));
					}).Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
				}
			}
			if (prevb0 != b[0] || prevb1 != b[1]) {
				NSHandler([p](NSThread*) {
					MouseMoved(p);
				}).Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
			}
			if (scroll_x != 0 || scroll_y != 0) {
				if (scroll_x == 2)
					scroll_x = -1;
				if (scroll_y == 2)
					scroll_y = -1;
				NSHandler([p, scroll_x, scroll_y](NSThread*) {
					MouseScrolled(p, scroll_x, scroll_y);
				}).Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
			}
		}
		prevb0 = b[0];
		prevb1 = b[1];
		for (int z = 0; z < 3; z++)
			prev_down[z] = down[z];
	}
	return NULL;
}

void* Desktop::KeyThread(void*) {
	int fd = open("/dev/keyboard", O_RDONLY);
	for (;;) {
		uint8_t buffer[2];
		read(fd, buffer, 2);
		
		bool down = buffer[0];
		uint8_t key = buffer[1];
		
		if (loaded) {
			if (down) {
				NSHandler([key](NSThread*) {
					KeyDown(key);
				}).Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
			} else {
				NSHandler([key](NSThread*) {
					KeyUp(key);
				}).Post(NSThread::MainThread(), NSHandlerDefaultPriortiy);
			}
		}
	}
	return NULL;
}

void Desktop::Start(NSThread*) {
	// Set the cursor
	graphics_cursor_image_set(0, 0, int(cursor_image->GetSize().width + 0.5f), int(cursor_image->GetSize().height + 0.5f),
							  const_cast<void*>(reinterpret_cast<const void*>(cursor_image->GetPixelData())));
	delete cursor_image;
	
	// Play the boot sound
	intro_sound->Play();
	
	// Start the fade timer
	gettimeofday(&fade_start, NULL);
	NSTimer::Create(FadeFunc, 1.0 / 60, true);
}

void Desktop::FadeFunc(NSTimer* timer) {
	int res_x = graphics_info.resolution_x;
	int res_y = graphics_info.resolution_y;
	
	// From 1 to 0
	struct timeval end;
	gettimeofday(&end, NULL);
	float fade = ((fade_start.tv_sec * 1000 * 1000 + fade_start.tv_usec + (1000 * 1000) -
				   (end.tv_sec * 1000 * 1000 + end.tv_usec)) / (1000.0f * 1000.0f));
	if (fade <= 0.0f)
		fade = 0.0f;
	constexpr float menu_fade_length = 0.25;
	
	NSMatrix matrix = NSMatrix::Identity();
	matrix.Scale(res_x, res_y, 1);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
	
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, background_vbo);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, background_vao, 3);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, fade_vao, 2);
	
	// Draw menus moving in
	if (fade <= menu_fade_length) {
		float fade2 = fade / menu_fade_length;
		NSRect system_menu_rect = system_menu->GetFrame();
		system_menu_rect.origin.y = (-system_menu_rect.size.height - 1) * fade2;
		system_menu->SetFrame(system_menu_rect);
		NSRect app_menu_rect = app_menu->GetFrame();
		app_menu_rect.origin.y = -(app_menu_rect.size.height - 1) * fade2;
		app_menu->SetFrame(app_menu_rect);
		NSRect task_bar_rect = task_bar->GetFrame();
		task_bar_rect.origin.y = res_y - menu_height + 1 + (task_bar_rect.size.height + 1) * fade2;
		task_bar->SetFrame(task_bar_rect);
		
		NSMenu* menus[] = { system_menu, app_menu, task_bar };
		std::vector<NSRect> rects = { NSRect(0, 0, res_x, res_y) };
		for (unsigned int z = 0; z < sizeof(menus) / sizeof(NSMenu*); z++)
			menus[z]->Draw(rects);
	}
	
	graphics_present(context.color_surface, 0, 0, 0, 0, res_x * Desktop::GetPixelScalingFactor(),
					 res_y * Desktop::GetPixelScalingFactor());
	
	// Update fade color
	for (int z = 0; z < 4; z++)
		fade_colors[z * 4 + 3] = fade;
	graphics_buffer_data(fade_vbo, fade_colors, sizeof(fade_colors));
	
	graphics_fence_sync(graphics_fence_create());
	
	if (fade <= 0.0f) {
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
		graphics_buffer_destroy(fade_vbo);
		timer->Invalidate();
		
		// Set correct menu placements
		NSRect system_menu_rect = system_menu->GetFrame(), app_menu_rect = app_menu->GetFrame(),
			task_bar_rect = task_bar->GetFrame();
		system_menu_rect.origin.y = 0;
		system_menu->SetFrame(system_menu_rect);
		app_menu_rect.origin.y = 0;
		app_menu->SetFrame(app_menu_rect);
		task_bar_rect.origin.y = res_y - menu_height + 1;
		task_bar->SetFrame(task_bar_rect);
		
		UpdateRect(NSRect(0, 0, res_x, res_y));
	}
}

// Helper to check if a rect intersects a rect vector
bool Desktop::RectsIntersect(const vector<NSRect>& rects, NSRect rect) {
	for (uint32_t z = 0; z < rects.size(); z++) {
		if (rects[z].OverlapsRect(rect))
			return true;
	}
	return false;
}

void Desktop::Draw(NSThread*) {
	rect_lock.Lock();
	if (dirty_rects.size() == 0) {
		rect_lock.Unlock();
		return;
	}
	vector<NSRect> rects = dirty_rects;
	dirty_rects.clear();
	update_requested = false;
	rect_lock.Unlock();
	
	int res_x = graphics_info.resolution_x;
	int res_y = graphics_info.resolution_y;
	
	if (RectsIntersect(rects, NSRect(0, 0, res_x, res_y))) {
		NSMatrix matrix = NSMatrix::Identity();
		matrix.Scale(res_x, res_y, 1);
		graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, background_vbo);
		graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, background_vao, 3);
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
	}
	
	// Figure out what intersects this rectangle
	NSRect mouse_rect = NSRectCorrected(NSRect(mouse_down_pos, NSSize(mouse_pos.x - mouse_down_pos.x,
																	  mouse_pos.y - mouse_down_pos.y)));
	if (mouse_desktop_down && RectsIntersect(rects, mouse_rect)) {
		NSMatrix matrix = NSMatrix::Identity();
		matrix.Translate(mouse_rect.origin.x, mouse_rect.origin.y, 0);
		matrix.Scale(mouse_rect.size.width, mouse_rect.size.height, 1);
		graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
		graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, mouse_vao, 2);
	}
	
	Window::Draw(rects);
	
	// Draw menus
	NSMenu* menus[] = { system_menu, app_menu, task_bar };
	for (unsigned int z = 0; z < sizeof(menus) / sizeof(NSMenu*); z++)
		menus[z]->Draw(rects);
	
	// TODO: get multi rect version of this
	float psf = Desktop::GetPixelScalingFactor();
	for (uint32_t z = 0; z < rects.size(); z++) {
		NSRect rect = rects[z] - BORDER_RECT / 2;
		if (!NSRectClamp(rect, NSRect(0, 0, res_x, res_y), &rect))
			continue;
		graphics_present(context.color_surface, rect.origin.x * psf, rect.origin.y * psf,
						 rect.origin.x * psf, rect.origin.y * psf,
						 round(rect.size.width * psf + 0.5), round(rect.size.height * psf + 0.5));
	}
}

void Desktop::UpdateRect(NSRect rect) {
	std::vector<NSRect> rects = { rect };
	UpdateRects(rects);
}

void Desktop::UpdateRects(const std::vector<NSRect>& rects) {
	std::vector<NSRect> real;
	real.reserve(rects.size());
	
	// TODO: remove redundant rects
	
	for (uint32_t z = 0; z < rects.size(); z++) {
		NSRect rect = NSRectCorrected(rects[z]).IntegerRect() + BORDER_RECT;
		real.emplace_back(NSRectCorrected(rect));
	}
	if (real.size() == 0)
		return;
	
	rect_lock.Lock();
	dirty_rects.insert(dirty_rects.end(), real.begin(), real.end());
	if (!update_requested) {
		NSHandler(Draw).Post(NSThread::MainThread(), 1 / 60.0f, 0, false);
		update_requested = true;
	}
	rect_lock.Unlock();
}

bool Desktop::MouseMenu(NSPoint p, NSMouseType type, NSMouseButton mouse, bool (NSMenu::*func)(NSEventMouse*)) {
	NSRect system_menu_rect = NSRect(NSPoint(graphics_info.resolution_x, 0),
									 NSSize(system_menu->GetSize().width, menu_height));
	system_menu_rect.origin.x -= system_menu_rect.size.width;
	NSRect app_menu_rect = NSRect(NSPoint(), NSSize(system_menu_rect.origin.x, menu_height));
	
	bool ret = false;
	NSEventMouse* event = NSEventMouse::Create(p - system_menu_rect.origin, type, mouse);
	ret = (system_menu->*func)(event);
	delete event;
	
	event = NSEventMouse::Create(p - app_menu_rect.origin, type, mouse);
	ret |= (app_menu->*func)(event);
	delete event;
	
	event = NSEventMouse::Create(p - task_bar->GetFrame().origin, type, mouse);
	ret |= (task_bar->*func)(event);
	delete event;
	
	return ret;
}

bool Desktop::KeyMenu(unsigned char key, NSModifierFlags flags, bool down, bool (NSMenu::*func)(NSEventKey*)) {
	bool ret = false;
	NSEventKey* event = NSEventKey::Create(key, down, flags);
	ret = (system_menu->*func)(event);
	ret |= (app_menu->*func)(event);
	ret |= (task_bar->*func)(event);
	delete event;
	
	return ret;
}

void Desktop::MouseDown(NSPoint p, NSMouseButton mouse) {
	p /= pixel_scaling_factor;
	
	if (mouse == NSMouseButtonLeft) {
		mouse_down = true;
		mouse_down_pos = p;
	}
	
	if (MouseMenu(p, NSMouseTypeDown, mouse, &NSMenu::MouseDown)) {
		mouse_menu_down = true;
		return;
	}
	
	if (Window::MouseDown(p, mouse))
		return;
	
	if (mouse == NSMouseButtonLeft)
		mouse_desktop_down = true;
}

void Desktop::MouseMoved(NSPoint p) {
	p /= pixel_scaling_factor;
	
	NSPoint old_pos = mouse_pos;
	mouse_pos = p;
	
	if ((mouse_down == mouse_menu_down) || mouse_menu_down) {
		MouseMenu(p, NSMouseTypeMoved, NSMouseButtonNone, &NSMenu::MouseMoved);
		return;
	}
	
	if (Window::MouseMoved(p))
		return;
	
	if (mouse_desktop_down) {
		// Update the difference
		NSPoint diff = mouse_pos - old_pos;
		std::vector<NSRect> rects = { NSRect(old_pos.x, mouse_down_pos.y, diff.x, old_pos.y - mouse_down_pos.y),
			NSRect(mouse_down_pos.x, old_pos.y, old_pos.x - mouse_down_pos.x, diff.y),
			NSRect(old_pos.x, old_pos.y, diff.x, diff.y) };
		UpdateRects(rects);
	}
}

void Desktop::MouseUp(NSPoint p, NSMouseButton mouse) {
	p /= pixel_scaling_factor;
	
	if (!mouse_down)
		return;
	mouse_down = false;
	
	if (mouse_menu_down) {
		mouse_menu_down = false;
		MouseMenu(p, NSMouseTypeUp, mouse, &NSMenu::MouseUp);
		return;
	}

	if (Window::MouseUp(p, mouse))
		return;
	
	if (mouse == NSMouseButtonLeft && mouse_desktop_down) {
		mouse_desktop_down = false;
		NSSize size = NSSize(mouse_pos.x - mouse_down_pos.x, mouse_pos.y - mouse_down_pos.y);
		if (size.width != 0 && size.height != 0)
			UpdateRect(NSRect(mouse_down_pos, size));
	}
}

void Desktop::MouseScrolled(NSPoint p, float delta_x, float delta_y) {
	p /= pixel_scaling_factor;

	if (Window::MouseScrolled(p, delta_x, delta_y))
		return;
}

void Desktop::KeyDown(unsigned char key) {
	if (key == NSKeyCodeLeftControl || key == NSKeyCodeRightControl)
		modifier_flags |= NSModifierControl;
	if (key == NSKeyCodeLeftAlt || key == NSKeyCodeRightAlt)
		modifier_flags |= NSModifierOption;
	if (key == NSKeyCodeLeftShift || key == NSKeyCodeRightShift)
		modifier_flags |= NSModifierShift;
	if (key == NSKeyCodeLeftCommand || key == NSKeyCodeRightCommand)
		modifier_flags |= NSModifierCommand;
	if (key == NSKeyCodeCapsLock && !caps_lock_down) {
		caps_lock_down = true;
		caps_lock_activated = !caps_lock_activated;
	}
	if (((modifier_flags & NSModifierShift) != 0) ^ caps_lock_activated)
		key = toupper(key);
	
	if (key == '1')
		SetPixelScalingFactor(1);
	else if (key == '2')
		SetPixelScalingFactor(2);
	else if (key == '3')
		SetPixelScalingFactor(3);
	
	if (KeyMenu(key, modifier_flags, true, &NSMenu::KeyDown))
		return;
	
	if (Window::KeyDown(key, modifier_flags))
		return;
}

void Desktop::KeyUp(unsigned char key) {
	if (key == NSKeyCodeLeftControl || key == NSKeyCodeRightControl)
		modifier_flags &= ~NSModifierControl;
	if (key == NSKeyCodeLeftAlt || key == NSKeyCodeRightAlt)
		modifier_flags &= ~NSModifierOption;
	if (key == NSKeyCodeLeftShift || key == NSKeyCodeRightShift)
		modifier_flags &= ~NSModifierShift;
	if (key == NSKeyCodeLeftCommand || key == NSKeyCodeRightCommand)
		modifier_flags &= ~NSModifierCommand;
	if (key == NSKeyCodeCapsLock)
		caps_lock_down = false;
	
	if (KeyMenu(key, modifier_flags, false, &NSMenu::KeyUp))
		return;
	
	if (Window::KeyUp(key, modifier_flags))
		return;
}

bool Desktop::IsMouseDown()	{
	return mouse_down;
}

void Desktop::RegisterApplication(Application::App* app) {
	NSMenuItem* item = new NSMenuItem(app->name);
	item->SetAction([app](NSMenuItem*) {
		if (app->windows.size() != 0)
			Window::MakeKeyWindow(app->pid, app->windows[0]);
	});
	item->SetUserData(reinterpret_cast<void*>(app));
	task_bar->AddItem(item);
	task_bar->AddItem(NSMenuItem::SeparatorItem());
}

void Desktop::UnregisterApplication(Application::App* app) {
	void* ptr = reinterpret_cast<void*>(app);
	auto items = task_bar->GetItems();
	for (unsigned int z = 0; z < items.size(); z++) {
		if (items[z]->GetUserData() == ptr) {
			task_bar->RemoveItem(z);
			task_bar->RemoveItem(z);	// Separator item
			break;
		}
	}
}

void Desktop::UpdateMenu() {
	NSMenu* menu = Application::GetActiveApplication()->menu;
	if (!menu)
		return;
	
	app_menu = menu;
	NSRect system_menu_rect = NSRect(NSPoint(graphics_info.resolution_x, 0),
									 NSSize(system_menu->GetSize().width, menu_height - 1));	// Account for border
	system_menu_rect.origin.x -= system_menu_rect.size.width;
	
	NSRect app_menu_rect = NSRect(NSPoint(), NSSize(system_menu_rect.origin.x, menu_height - 1));
	app_menu->SetFrame(app_menu_rect);
	app_menu->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	app_menu->SetContext(&context);
	
	Desktop::UpdateRect(app_menu_rect + NSRect(-1, -1, 2, 2));
}
