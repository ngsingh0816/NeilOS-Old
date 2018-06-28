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

#include "Events/Application/NSEventApplicationMenuEvent.cpp"

#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <vector>

#define BORDER_RECT		NSRect(-2, -2, 4, 4)
#define CURSOR_SIZE		20

#define DOUBLE_CLICK_TIME		0.5

namespace NSApplication {
	void SetPixelScalingFactor(float factor);
}

using std::vector;

graphics_info_t graphics_info;
graphics_context_t context;

namespace Desktop {
	bool loaded = false;
	bool done_fade = false;
	
	NSImage* cursor_image[NSCursor::CURSOR_MAX];
	NSPoint cursor_hotspot[NSCursor::CURSOR_MAX];
	NSSound* intro_sound = NULL;
	
	NSMenu* app_menu = NULL;		// on the left
	NSMenu* system_menu = NULL;		// on the right
	NSMenu* task_bar = NULL;		// on the bottom
	NSMenu* context_menu = NULL;
	uint32_t context_menu_pid = 0;
	uint32_t context_menu_id = 0;
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
	float original_psf = 2.0f;
	
	NSCursor::Cursor current_cursor = NSCursor::CURSOR_MAX;
	
	struct timeval fade_start;
	
	NSSize AdjustCursorSize(NSSize size);
	
	void* MouseThread(void*);
	void* KeyThread(void*);
	void FadeFunc(NSTimer*);
	
	bool MouseMenu(NSPoint p, NSMouseType type, NSMouseButton mouse, uint32_t click_count,
				   bool (NSMenu::*func)(NSEventMouse*), float delta_x = 0, float delta_y = 0);
	bool KeyMenu(unsigned char key, NSModifierFlags flags, bool down, bool (NSMenu::*func)(NSEventKey*));
	
	
	void SetMenuFrames();
	
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
	
	NSApplication::SetPixelScalingFactor(factor);
	Application::SendPixelScalingFactorEvent(factor);
	
	SetMenuFrames();
	
	UpdateRect(NSRect(0, 0, graphics_info.resolution_x, graphics_info.resolution_y));
}

void Desktop::SetMenuFrames() {
	const NSRect bounds = NSRect(0, menu_height - 1, graphics_info.resolution_x,
								 graphics_info.resolution_y - (menu_height - 1) * 2);
	system_menu->SetFrameBounds(bounds);
	app_menu->SetFrameBounds(bounds);
	task_bar->SetFrameBounds(bounds);
	
	NSRect system_menu_rect = NSRect(NSPoint(graphics_info.resolution_x, -1),
									 NSSize(system_menu->GetSize().width + 1, menu_height + 1));
	system_menu_rect.origin.x -= system_menu_rect.size.width - 1;
	system_menu->SetFrame(system_menu_rect);
	NSRect app_menu_rect = NSRect(NSPoint(-1, -1), NSSize(system_menu_rect.origin.x + 2, menu_height + 1));
	app_menu->SetFrame(app_menu_rect);
	task_bar->SetFrame(NSRect(-1, graphics_info.resolution_y - menu_height,
							  graphics_info.resolution_x + 2, menu_height + 1));
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
	
	static const char* cursor_names[] = {
		"default", "ibeam", "move", "cross", "drag_and_drop", "screenshot", "open_hand", "closed_hand",
		"pointing_hand", "resize_north", "resize_northeast", "resize_northwest", "resize_south",
		"resize_southeast", "resize_southwest", "resize_northeast_southwest", "resize_northwest_southeast",
		"resize_north_south", "resize_east", "resize_west", "resize_east_west", "resize_up", "resize_down",
		"resize_up_down", "resize_left", "resize_right", "resize_left_right", "zoom_in", "zoom_out",
	};
	
	for (int z = 0; z < NSCursor::CURSOR_MAX; z++) {
		cursor_image[z] = new NSImage(std::string("/system/images/cursors/") +
								   std::string(cursor_names[z]) + std::string(".png"));
		NSSize old_size = cursor_image[z]->GetSize();
		cursor_image[z]->SetSize(AdjustCursorSize(cursor_image[z]->GetSize()));
		NSSize new_size = cursor_image[z]->GetSize();
		
		FILE* hotspot = fopen((std::string("/system/images/cursors/") +
							  std::string(cursor_names[z]) + std::string(".txt")).c_str(), "r");
		if (hotspot) {
			int x, y;
			fscanf(hotspot, "%i, %i\n", &x, &y);
			fclose(hotspot);
			cursor_hotspot[z] = NSPoint(x * (new_size.width / old_size.width), y * (new_size.height / old_size.height));
		} else
			cursor_hotspot[z] = NSPoint();
		
		percent(10 + (float(z + 1) / NSCursor::CURSOR_MAX) * 40);
	}
	
	NSImage* background_image = new NSImage("/system/images/background.jpg");
	
	percent(65);
	
	square_vbo = graphics_buffer_create(sizeof(float) * 4 * 2, GRAPHICS_BUFFER_STATIC);
	NSView::BufferSquare(square_vbo);
	color_vbo = graphics_buffer_create(sizeof(float) * 4 * 4, GRAPHICS_BUFFER_STATIC);
	NSView::BufferColor(color_vbo, NSColor<float>::WhiteColor(), 4);
	fade_vbo = graphics_buffer_create(sizeof(float) * 4 * 4, GRAPHICS_BUFFER_DYNAMIC);
	NSView::BufferColor(fade_vbo, NSColor<float>::BlackColor(), 4);
	uv_vbo = graphics_buffer_create(sizeof(float) * 4 * 2, GRAPHICS_BUFFER_STATIC);
	NSView::BufferSquare(uv_vbo);
	mouse_vbo = graphics_buffer_create(sizeof(float) * 4 * 4, GRAPHICS_BUFFER_STATIC);
	NSView::BufferColor(mouse_vbo, NSColor<float>(0.5, 0.5, 0.5, 0.5), 4);

	background_vbo = NSView::CreateImageBuffer(background_image, NULL);
	delete background_image;
	
	memset(background_vao, 0, sizeof(graphics_vertex_array_t) * 3);
	background_vao[0].bid = square_vbo;
	background_vao[0].stride = 2 * sizeof(float);
	background_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	background_vao[0].usage = GRAPHICS_USAGE_POSITION;
	background_vao[1].bid = color_vbo;
	background_vao[1].stride = 4 * sizeof(float);
	background_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	background_vao[1].usage = GRAPHICS_USAGE_COLOR;
	background_vao[2].bid = uv_vbo;
	background_vao[2].stride = 2 * sizeof(float);
	background_vao[2].type = GRAPHICS_TYPE_FLOAT2;
	background_vao[2].usage = GRAPHICS_USAGE_TEXCOORD;
	
	memset(fade_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	fade_vao[0].bid = square_vbo;
	fade_vao[0].stride = 2 * sizeof(float);
	fade_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	fade_vao[0].usage = GRAPHICS_USAGE_POSITION;
	fade_vao[1].bid = fade_vbo;
	fade_vao[1].stride = 4 * sizeof(float);
	fade_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	fade_vao[1].usage = GRAPHICS_USAGE_COLOR;
	
	memset(mouse_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	mouse_vao[0].bid = square_vbo;
	mouse_vao[0].stride = 2 * sizeof(float);
	mouse_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	mouse_vao[0].usage = GRAPHICS_USAGE_POSITION;
	mouse_vao[1].bid = mouse_vbo;
	mouse_vao[1].stride = 4 * sizeof(float);
	mouse_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	mouse_vao[1].usage = GRAPHICS_USAGE_COLOR;
		
	pthread_create(&mouse_thread, NULL, MouseThread, NULL);
	pthread_create(&key_thread, NULL, KeyThread, NULL);
	
	percent(70);
	
	app_menu = new NSMenu;
	system_menu = new NSMenu;
	task_bar = new NSMenu;
	
	// TODO: temp
	NSMenuItem* item = new NSMenuItem("File");
	item->SetSubmenu(new NSMenu);
	item->GetSubmenu()->AddItem(new NSMenuItem("Open"));
	item->GetSubmenu()->AddItem(new NSMenuItem("Close"));
	item->GetSubmenu()->GetItems()[1]->SetIsEnabled(false);
	NSTextField* slider = NSTextField::Create(NSRect(NSPoint(), NSTextFieldDefaultSize));
	slider->SetResizingMask(NSViewWidthSizable | NSViewMinXMargin | NSViewMaxXMargin);
	item->GetSubmenu()->AddItem(new NSMenuViewItem(slider));
	item->GetSubmenu()->AddItem(new NSMenuItem("Save"));
	item->GetSubmenu()->GetItems()[2]->SetKeyEquivalent("s", NSModifierCommand);
	item->GetSubmenu()->GetItems()[2]->SetState(NSMenuItemStateOn);
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
	app_menu->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	
	int audio_fd = open("/dev/audio", O_RDONLY);
	NSSlider* audio_slider = NSSlider::Create(NSRect(NSPoint(), NSSliderDefaultSize));
	audio_slider->SetResizingMask(NSViewWidthSizable);
	audio_slider->SetMinValue(0);
	audio_slider->SetMaxValue(100);
	audio_slider->SetValue(100);
	audio_slider->SetIsContinuous(true);
	audio_slider->SetAction([audio_fd](NSSlider* slider) {
		double v = (slider->GetValue() - slider->GetMinValue()) / (slider->GetMaxValue() - slider->GetMinValue());
		// Set master volume
		ioctl(audio_fd, 2, v);
	});
	NSImage* audio_image = new NSImage("/system/images/audio.png");
	audio_image->SetScaledSize(NSSize(menu_height - 10, menu_height - 10));
	NSMenuItem* audio_menu = new NSMenuItem(audio_image, false);
	audio_menu->SetBorderHeight(2);
	delete audio_image;
	NSMenuItem* slider_item = new NSMenuViewItem(audio_slider);
	audio_menu->SetSubmenu(new NSMenu);
	audio_menu->GetSubmenu()->AddItem(slider_item);
	system_menu->AddItem(audio_menu);
	NSMenuItem* time_item = new NSMenuItem(GetTime());
	system_menu->AddItem(time_item);
	system_menu->AddItem(new NSMenuItem("NeilOS"));		// TODO: replace with image?
	system_menu->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	system_menu->SetHighlightedTextColor(NSColor<float>::WhiteColor());
	
	
	percent(90);
	
	NSImage* start_image = new NSImage("/system/images/start.png");
	start_image->SetScaledSize(NSSize(menu_height - 5, menu_height - 5));
	item = new NSMenuItem(start_image, false);
	delete start_image;
	item->SetBorderHeight(2);
	NSMenuItem* sitem = new NSMenuItem("Calculator");
	sitem->SetFont(NSFont(30));
	sitem->SetAction([](NSMenuItem*) {
		NSApplication::OpenApplication("/Applications/Calculator");
	});
	item->SetSubmenu(new NSMenu);
	item->GetSubmenu()->AddItem(sitem);
	sitem = new NSMenuItem("Logger");
	sitem->SetFont(NSFont(30));
	sitem->SetAction([](NSMenuItem*) {
		NSApplication::OpenApplication("/Applications/Logger");
	});
	item->GetSubmenu()->AddItem(sitem);
	task_bar->AddItem(item);
	task_bar->AddItem(NSMenuItem::SeparatorItem());
	task_bar->SetOrientation(NSMenuOrientationUp);
	task_bar->SetHighlightColor(NSColor<float>::UIDarkGrayColor());
	task_bar->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	
	SetMenuFrames();
	app_menu->SetContext(&context);
	system_menu->SetContext(&context);
	task_bar->SetContext(&context);
	
	percent(98);
		
	// Create time timer
	NSTimer* timer = NSTimer::Create([time_item](NSTimer*) {
		time_item->SetTitle(GetTime());
		NSTimer::Create([time_item](NSTimer*) {
			time_item->SetTitle(GetTime());
		}, 60, true);
	}, SecondsToNextMinute(), false, false);
	NSThread::MainThread()->GetRunLoop()->AddTimer(timer);
	
	loaded = true;
	percent(100);
	
#undef percent
}

void* Desktop::MouseThread(void*) {
	int fd = open("/dev/mouse", O_RDONLY);
	//int key_fd = open("/dev/keyboard", O_RDONLY);
	
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
		int scroll_y = (buffer[8] >> 5) & 0x3;
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
				if (scroll_x == 0x3)
					scroll_x = -1;
				if (scroll_y == 0x3)
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
	Desktop::SetCursor(NSCursor::CURSOR_DEFAULT);
	
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
		system_menu_rect.origin.y = -system_menu_rect.size.height * fade2 - 1;
		system_menu->SetFrame(system_menu_rect);
		NSRect app_menu_rect = app_menu->GetFrame();
		app_menu_rect.origin.y = -app_menu_rect.size.height * fade2 - 1;
		app_menu->SetFrame(app_menu_rect);
		NSRect task_bar_rect = task_bar->GetFrame();
		task_bar_rect.origin.y = res_y - menu_height + (task_bar_rect.size.height - 1) * fade2;
		task_bar->SetFrame(task_bar_rect);
		
		NSMenu* menus[] = { system_menu, app_menu, task_bar };
		std::vector<NSRect> rects = { NSRect(0, 0, res_x, res_y) };
		for (unsigned int z = 0; z < sizeof(menus) / sizeof(NSMenu*); z++)
			menus[z]->Draw(rects);
	}
	
	graphics_present(context.color_surface, 0, 0, 0, 0, res_x * Desktop::GetPixelScalingFactor(),
					 res_y * Desktop::GetPixelScalingFactor());
	
	// Update fade color
	NSView::BufferColor(fade_vbo, NSColor<float>::BlackColor().AlphaColor(fade), 4);
	
	graphics_fence_sync(graphics_fence_create());
	
	if (fade <= 0.0f) {
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
		graphics_buffer_destroy(fade_vbo);
		timer->Invalidate();
		
		// Set correct menu placements
		NSRect system_menu_rect = system_menu->GetFrame(), app_menu_rect = app_menu->GetFrame(),
			task_bar_rect = task_bar->GetFrame();
		system_menu_rect.origin.y = -1;
		system_menu->SetFrame(system_menu_rect);
		app_menu_rect.origin.y = -1;
		app_menu->SetFrame(app_menu_rect);
		task_bar_rect.origin.y = res_y - menu_height;
		task_bar->SetFrame(task_bar_rect);
		
		done_fade = true;
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
	if (context_menu)
		context_menu->Draw(rects);
	
	// TODO: get multi rect version of this
	std::vector<NSRect> consolidated_rects;
	consolidated_rects.reserve(rects.size());
	for (uint32_t z = 0; z < rects.size(); z++) {
		NSRect rect = rects[z] - BORDER_RECT / 2;
		if (!NSRectClamp(rect, NSRect(0, 0, res_x, res_y), &rect))
			continue;
		consolidated_rects.emplace_back(rect);
	}
	consolidated_rects = NSRectConsolidate(consolidated_rects);
	float psf = Desktop::GetPixelScalingFactor();
	for (uint32_t z = 0; z < consolidated_rects.size(); z++) {
		const NSRect& rect = consolidated_rects[z];
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
	if (!done_fade)
		return;
	
	std::vector<NSRect> real;
	real.reserve(rects.size());
	
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

bool Desktop::MouseMenu(NSPoint p, NSMouseType type, NSMouseButton mouse, uint32_t click_count,
						bool (NSMenu::*func)(NSEventMouse*), float delta_x, float delta_y) {
	NSRect system_menu_rect = NSRect(NSPoint(graphics_info.resolution_x, 0),
									 NSSize(system_menu->GetSize().width, menu_height));
	system_menu_rect.origin.x -= system_menu_rect.size.width;
	NSRect app_menu_rect = NSRect(NSPoint(), NSSize(system_menu_rect.origin.x, menu_height));
	
	bool ret = false;
	NSEventMouse* event = NSEventMouse::Create(p - system_menu_rect.origin, type, mouse, click_count);
	event->SetDeltaX(delta_x);
	event->SetDeltaY(delta_y);
	ret = (system_menu->*func)(event);
	delete event;
	
	event = NSEventMouse::Create(p - app_menu_rect.origin, type, mouse, click_count);
	event->SetDeltaX(delta_x);
	event->SetDeltaY(delta_y);
	ret |= (app_menu->*func)(event);
	delete event;
	
	event = NSEventMouse::Create(p - task_bar->GetFrame().origin, type, mouse, click_count);
	event->SetDeltaX(delta_x);
	event->SetDeltaY(delta_y);
	ret |= (task_bar->*func)(event);
	delete event;
	
	if (context_menu) {
		event = NSEventMouse::Create(p - context_menu->GetFrame().origin, type, mouse, click_count);
		event->SetDeltaX(delta_x);
		event->SetDeltaY(delta_y);
		bool context_ret = (context_menu->*func)(event);
		delete event;
		
		if ((type == NSMouseTypeDown && !context_ret) || (type == NSMouseTypeUp && context_ret))
			PopupContextMenu(context_menu_pid, NULL, NSPoint(), 0);
		
		ret |= context_ret;
	}
	
	return ret;
}

bool Desktop::KeyMenu(unsigned char key, NSModifierFlags flags, bool down, bool (NSMenu::*func)(NSEventKey*)) {
	bool ret = false;
	NSEventKey* event = NSEventKey::Create(key, down, flags);
	ret = (system_menu->*func)(event);
	ret |= (app_menu->*func)(event);
	ret |= (task_bar->*func)(event);
	if (context_menu)
		ret |= (context_menu->*func)(event);
	delete event;
	
	return ret;
}

void Desktop::MouseDown(NSPoint p, NSMouseButton mouse) {
	// Find time since last click
	static timeval last = { .tv_sec = 0, .tv_usec = 0 };
	static unsigned int click_count = 0;
	static NSMouseButton last_button = NSMouseButtonNone;
	struct timeval current;
	gettimeofday(&current, NULL);
	NSTimeInterval t = (current.tv_sec - last.tv_sec) + ((current.tv_usec - last.tv_usec) / (1000.0 * 1000.0));
	if (t < DOUBLE_CLICK_TIME && last_button == mouse) {
		click_count++;
	} else
		click_count = 1;
	last = current;
	last_button = mouse;
	
	p /= pixel_scaling_factor;
	
	if (mouse == NSMouseButtonLeft) {
		mouse_down = true;
		mouse_down_pos = p;
	}
	
	if (MouseMenu(p, NSMouseTypeDown, mouse, click_count, &NSMenu::MouseDown)) {
		mouse_menu_down = true;
		return;
	}
	
	if (Window::MouseDown(p, mouse, click_count))
		return;
	
	if (mouse == NSMouseButtonLeft)
		mouse_desktop_down = true;
}

void Desktop::MouseMoved(NSPoint p) {
	p /= pixel_scaling_factor;
	
	NSPoint old_pos = mouse_pos;
	mouse_pos = p;
	
	if (mouse_desktop_down) {
		// Update the difference
		NSPoint diff = mouse_pos - old_pos;
		std::vector<NSRect> rects = { NSRect(old_pos.x, mouse_down_pos.y, diff.x, old_pos.y - mouse_down_pos.y),
			NSRect(mouse_down_pos.x, old_pos.y, old_pos.x - mouse_down_pos.x, diff.y),
			NSRect(old_pos.x, old_pos.y, diff.x, diff.y) };
		UpdateRects(rects);
		return;
	}
	
	if ((mouse_down == mouse_menu_down) || mouse_menu_down) {
		if (MouseMenu(p, NSMouseTypeMoved, NSMouseButtonNone, 0, &NSMenu::MouseMoved))
			return;
	}
	
	if (Window::MouseMoved(p))
		return;
}

void Desktop::MouseUp(NSPoint p, NSMouseButton mouse) {
	p /= pixel_scaling_factor;
	
	if (!mouse_down)
		return;
	mouse_down = false;
	
	if (mouse_menu_down) {
		mouse_menu_down = false;
		MouseMenu(p, NSMouseTypeUp, mouse, 0, &NSMenu::MouseUp);
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
#define TIMER_CUTOFF		0.5
#define SCROLL_SENSITIVITY	2
	
	p /= pixel_scaling_factor;
	
	static timeval last = { 0, 0 };
	
	struct timeval current;
	gettimeofday(&current, NULL);
	
	NSTimeInterval t = (current.tv_sec - last.tv_sec) + ((current.tv_usec - last.tv_usec) / (1000.0 * 1000.0));
	last = current;
		
	if (t < TIMER_CUTOFF) {
		t = SCROLL_SENSITIVITY * (TIMER_CUTOFF - t) / TIMER_CUTOFF;
		t = t * t * t * t * t * t;
		delta_x *= t;
		delta_y *= t;
	}
	
	if (MouseMenu(p, NSMouseTypeScrolled, NSMouseButtonNone, 0, &NSMenu::MouseScrolled, delta_x, delta_y))
		return;

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
	if ((modifier_flags & NSModifierShift) != 0) {
		static uint8_t shift_map[] = {
			')', '!', '@', '#', '$', '%', '^', '&', '*', '('
		};
		if (key >= '0' && key <= '9')
			key = shift_map[key - '0'];
		switch (key) {
			case '`':
				key = '~';
				break;
			case '-':
				key = '_';
				break;
			case '=':
				key = '+';
				break;
			case '[':
				key = '{';
				break;
			case ']':
				key = '}';
				break;
			case '\\':
				key = '|';
				break;
			case '\'':
				key = '"';
				break;
			case ';':
				key = ':';
				break;
			case ',':
				key = '<';
				break;
			case '.':
				key = '>';
				break;
			case '/':
				key = '?';
				break;
		}
	}
	
	if (modifier_flags & NSModifierOption) {
		if (key == '1')
			SetPixelScalingFactor(1);
		else if (key == '2')
			SetPixelScalingFactor(2);
		else if (key == '3')
			SetPixelScalingFactor(3);
	}
	
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
	SetMenuFrames();
	app_menu->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	app_menu->SetContext(&context);
	
	Desktop::UpdateRect(app_menu->GetFullFrame());
}

NSSize Desktop::AdjustCursorSize(NSSize size) {
	float aspect = size.width / size.height;
	float new_height = CURSOR_SIZE * Desktop::GetPixelScalingFactor();
	return NSSize(new_height * aspect, new_height);
}

NSPoint Desktop::GetMousePos() {
	return mouse_pos;
}

void Desktop::SetCursor(NSCursor::Cursor cursor) {
	if (current_cursor == cursor)
		return;
	int z = static_cast<int>(cursor);
	graphics_cursor_image_set(round(cursor_hotspot[z].x), round(cursor_hotspot[z].y), int(cursor_image[z]->GetSize().width + 0.5f),
							  int(cursor_image[z]->GetSize().height + 0.5f),
							  const_cast<void*>(reinterpret_cast<const void*>(cursor_image[z]->GetPixelData())));
	current_cursor = cursor;
}

void Desktop::SetCursor(NSImage* image, NSPoint hotspot) {
	image->SetSize(AdjustCursorSize(image->GetSize()));
	graphics_cursor_image_set(round(hotspot.x), round(hotspot.y), int(image->GetSize().width + 0.5f),
							  int(image->GetSize().height + 0.5f),
							  const_cast<void*>(reinterpret_cast<const void*>(image->GetPixelData())));
	current_cursor = NSCursor::CURSOR_MAX;
}

void Desktop::PopupContextMenu(uint32_t pid, NSMenu* menu, const NSPoint& p, uint32_t context_id) {
	if (context_menu && (menu || context_menu_pid == pid)) {
		NSEventApplicationMenuEvent* e = NSEventApplicationMenuEvent::Create(std::vector<unsigned int>(),
																			 NULL, true, context_menu_id, true);
		Application::SendEvent(e, pid);
		delete e;
		
		NSMenu* copy = context_menu;
		context_menu = NULL;
		context_menu_pid = 0;
		context_menu_id = 0;
		copy->Clear();
		Desktop::UpdateRect(copy->GetFullFrame());
		delete copy;
	}
	if (!menu)
		return;
	
	context_menu = menu;
	context_menu_pid = pid;
	context_menu_id = context_id;
	context_menu->SetIsContextMenu(true);
	context_menu->SetFrameBounds(NSRect(0, menu_height, graphics_info.resolution_x,
										graphics_info.resolution_y - menu_height * 2));
	context_menu->SetFrame(NSRect(p, NSSize()));
	context_menu->SetUpdateFunction([](std::vector<NSRect> rects) {
		UpdateRects(rects);
	});
	context_menu->SetContext(&context);
	
	// Mouse moved event for having corrected highlighting
	NSEventMouse* e = NSEventMouse::Create(mouse_pos - context_menu->GetFrame().origin,
										   NSMouseTypeMoved, NSMouseButtonNone);
	context_menu->MouseMoved(e);
	delete e;
	
	Desktop::UpdateRect(context_menu->GetFullFrame());
}
