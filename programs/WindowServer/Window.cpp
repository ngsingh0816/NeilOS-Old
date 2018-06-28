//
//  Window.cpp
//  product
//
//  Created by Neil Singh on 2/19/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "Window.h"

#include "Desktop.h"
#include "Events/NSEventDefs.cpp"

#include <math.h>
#include <string.h>

#include <map>
#include <list>

#define WINDOW_CORNER_LEFT			(1 << 1)
#define WINDOW_CORNER_RIGHT			(1 << 2)
#define WINDOW_CORNER_TOP			(1 << 3)
#define WINDOW_CORNER_BOTTOM		(1 << 4)

#define NUM_BUTTONS				3
#define BUTTON_VERTICES			16
#define BUTTON_OFFSET			10.0
#define BUTTON_SPACING			6.0
#define BUTTON_WIDTH			13.0
#define BUTTON_BORDER_SIZE		1.0
#define BUTTON_INSET			4.0
#define BUTTON_SYMBOL_HEIGHT	1.0
#define TOTAL_BUTTON_SIZE	((2 * BUTTON_OFFSET) + (BUTTON_SPACING + BUTTON_WIDTH) * 3)

#define BUTTON_BORDER			6
#define BUTTON_UNHIGHLIGHTED	7
#define BUTTON_DISABLED			8

using std::string;

class WSWindow;

namespace Window {
	std::unordered_map<uint64_t, WSWindow*> windows;
	std::list<WSWindow*> window_list;
	
	constexpr float round_width = 5.0;
	constexpr int num_title_vertices = 22;	// must be multiple of 2 + 4 (and at least 4)
	
	uint32_t title_key_colors_vbo;
	uint32_t title_colors_vbo;
	uint32_t border_color_vbo;
	uint32_t square_vbo;
	uint32_t button_vbo;
	// (red, yellow, green) x 2, gray (border), light gray (unhighlighted), ligher gray (disabled)
	uint32_t button_colors_vbo[9];
	uint32_t button_text_color_vbo;
	graphics_vertex_array_t square_vao[3];
	graphics_vertex_array_t button_vao[2];
	graphics_vertex_array_t view_border_vao[2];
	
	// Events
	void CreateEvent(uint8_t* data, uint32_t length);
	void DestroyEvent(uint8_t* data, uint32_t length);
	void ShowEvent(uint8_t* data, uint32_t length);
	void MakeKeyEvent(uint8_t* data, uint32_t length);
	void SetTitleEvent(uint8_t* data, uint32_t length);
	void SetFrameEvent(uint8_t* data, uint32_t length);
	void SetButtonMaskEvent(uint8_t* data, uint32_t length);
	void DrawEvent(uint8_t* data, uint32_t length);
	
	// Drawing
	void DrawButtons(WSWindow* window, const std::vector<NSRect>& rects);
	
	// Mouse
	NSPoint mouse_pos;
	bool mouse_down = false;
	NSMouseButton mouse_down_type = NSMouseButtonNone;
	bool down_title = false;
	
	// Key window
	void MakeKeyWindow(WSWindow* key);
	WSWindow* key_window = NULL;
	
	// Cursor
	NSCursor::Cursor GetCursorForFrame(const NSRect& frame, const NSPoint& p);
	int GetCornersForFrame(const NSRect& frame, const NSPoint& p);
	
	void UpdateFrame(WSWindow* window, NSRect frame, bool delayed=false);
	
	inline uint64_t MakeKey(uint32_t pid, uint32_t id) {
		return (uint64_t(pid) << 32) | id;
	}
	
	inline uint32_t GetPidFromKey(uint64_t key) {
		return ((key >> 32) & 0xFFFFFFFF);
	}
	
	inline uint32_t GetWidFromKey(uint64_t key) {
		return (key & 0xFFFFFFFF);
	}
	
	WSWindow* GetWindow(uint32_t pid, uint32_t id) {
		uint64_t key = MakeKey(pid, id);
		auto it = windows.find(key);
		if (it == windows.end())
			return NULL;
		return it->second;
	}
}

class WSWindow {
public:
	string title;
	NSRect frame;
	uint32_t pid;
	uint32_t id;
	graphics_context_t context;
	
	uint32_t title_vbo;
	graphics_vertex_array_t title_vao[2];
	uint32_t text_img;
	NSSize text_size;
	float text_height;
	bool highlights_buttons = false;
#define BUTTON_MASK_CLOSE		(1 << 0)
#define BUTTON_MASK_MIN			(1 << 1)
#define BUTTON_MASK_MAX			(1 << 2)
#define BUTTON_MASK_ALL			((1 << NUM_BUTTONS) - 1)
	unsigned int button_mask = BUTTON_MASK_ALL;
	unsigned int button_down = 0;
	unsigned int button_over = 0;
	
	NSCursor::Cursor cursor = NSCursor::CURSOR_DEFAULT;
	bool resizing = false;
	int resizing_corners = 0;
	bool needs_frame_update = false;
	
	bool visible;
	
	inline NSRect GetTitleFrame() const {
		return NSRect(frame.origin.x, frame.origin.y, frame.size.width, WINDOW_TITLE_BAR_HEIGHT);
	}
	inline NSRect GetContentFrame() const {
		return NSRect(frame.origin.x, frame.origin.y + WINDOW_TITLE_BAR_HEIGHT, frame.size.width,
					  frame.size.height - WINDOW_TITLE_BAR_HEIGHT);
	}
	inline NSRect GetButtonFrame() const {
		return NSRect(frame.origin.x + BUTTON_OFFSET,
					  frame.origin.y + (WINDOW_TITLE_BAR_HEIGHT - BUTTON_WIDTH) / 2,
					  TOTAL_BUTTON_SIZE, BUTTON_WIDTH);
	}
	inline NSRect GetButtonFrame(unsigned int z) const {
		return NSRect(frame.origin.x + BUTTON_OFFSET + (BUTTON_SPACING + BUTTON_WIDTH) * z,
					  frame.origin.y + (WINDOW_TITLE_BAR_HEIGHT - BUTTON_WIDTH) / 2,
					  BUTTON_WIDTH, BUTTON_WIDTH);
	}
	inline NSSize GetMinimumSize() const {
		return NSSize(text_size.width + TOTAL_BUTTON_SIZE + BUTTON_OFFSET, WINDOW_TITLE_BAR_HEIGHT * 2);
	}
	
	WSWindow() {
		using namespace Window;
		
		memset(&context, 0, sizeof(context));
		memset(title_vao, 0, sizeof(title_vao));
		visible = false;
		
		title_vbo = graphics_buffer_create(sizeof(float) * num_title_vertices * 2, GRAPHICS_BUFFER_STATIC);
		
		title_vao[0].bid = title_vbo;
		title_vao[0].stride = 2 * sizeof(float);
		title_vao[0].type = GRAPHICS_TYPE_FLOAT2;
		title_vao[0].usage = GRAPHICS_USAGE_POSITION;
		title_vao[1].bid = title_colors_vbo;
		title_vao[1].stride = 4 * sizeof(float);
		title_vao[1].type = GRAPHICS_TYPE_FLOAT4;
		title_vao[1].usage = GRAPHICS_USAGE_COLOR;
	}
	
	~WSWindow() {
		if (title_vbo) {
			graphics_buffer_destroy(title_vbo);
			title_vbo = 0;
		}
		if (text_img) {
			graphics_buffer_destroy(text_img);
			text_img = 0;
		}
	}
	
	void SetTitle(string t) {
		title = t;
		
		if (text_img)
			graphics_buffer_destroy(text_img);
		
		NSFont font;
		NSImage* text = font.GetImage(title, NSColor<float>::BlackColor());
		if (text) {
			text_img = NSView::CreateImageBuffer(text, &text_size);
			text_height = font.GetLineHeight();
			delete text;
		} else
			text_img = 0;
		
		if (visible)
			Desktop::UpdateRect(GetTitleFrame());
		
		NSSize min_size = GetMinimumSize();
		if (min_size.width > frame.size.width) {
			NSRect f = frame;
			frame.size.width = min_size.width;
			SetFrame(f);
		}
	}
	
	void SetFrame(NSRect f, bool update=true) {
		NSSize min_size = GetMinimumSize();
		if (f.size.height < min_size.height)
			f.size.height = min_size.height;
		if (f.size.width < min_size.width)
			f.size.width = min_size.width;
		
		NSRect old_frame = frame;
		frame = f;
		
		if (old_frame.size != frame.size)
			needs_frame_update = true;
			
		if (visible && update) {
			Desktop::UpdateRect(old_frame);
			Desktop::UpdateRect(frame);
		}
	}
	
	void SetHighlightsButtons(bool h) {
		if (highlights_buttons != h && visible)
			Desktop::UpdateRect(GetButtonFrame());
		highlights_buttons = h;
	}
	
	void SetButtonDown(unsigned int index, bool d) {
		unsigned int prev = button_down;
		if (d) {
			button_down |= (1 << index);
			button_over |= (1 << index);
		}
		else {
			button_down &= ~(1 << index);
			button_over &= ~(1 << index);
		}
		if (prev != button_down)
			Desktop::UpdateRect(GetButtonFrame(index));
	}
	
	void SetButtonOver(unsigned int index, bool o) {
		unsigned int prev = button_over;
		if (o)
			button_over |= (1 << index);
		else
			button_over &= ~(1 << index);
		if (prev != button_over)
			Desktop::UpdateRect(GetButtonFrame(index));
	}
	
	void UpdateFrameIfNeeded() {
		using namespace Window;

		if (!needs_frame_update)
			return;
		needs_frame_update = false;
		float borders[] = { round_width, round_width, 0, 0 };
		NSView::BufferRoundedRect(title_vbo, NSSize(frame.size.width, WINDOW_TITLE_BAR_HEIGHT), borders,
								  NSVIEW_ROUNDED_UPPER_LEFT | NSVIEW_ROUNDED_UPPER_RIGHT, num_title_vertices);
	}
};

void Window::Load(volatile float* p, float percent_start, float percent_end) {
#define percent(x) *p = percent_start + (percent_end - percent_start) / 100.0f * x
	
	percent(0);
	
	title_key_colors_vbo = graphics_buffer_create(sizeof(float) * num_title_vertices * 4 , GRAPHICS_BUFFER_STATIC);
	NSView::BufferColor(title_key_colors_vbo, NSColor<float>::UILightGrayColor(), num_title_vertices);
	title_colors_vbo = graphics_buffer_create(sizeof(float) * num_title_vertices * 4, GRAPHICS_BUFFER_STATIC);
	NSView::BufferColor(title_colors_vbo, NSColor<float>::UILighterGrayColor(), num_title_vertices);
	
	square_vbo = graphics_buffer_create(sizeof(float) * 4 * 2, GRAPHICS_BUFFER_STATIC);
	NSView::BufferSquare(square_vbo);
	
	button_vbo = graphics_buffer_create(sizeof(float) * BUTTON_VERTICES * 2, GRAPHICS_BUFFER_STATIC);
	NSView::BufferOval(button_vbo, BUTTON_VERTICES);
	NSColor<float> bcolors[] = { NSColor<uint8_t>(252, 97, 93), NSColor<uint8_t>(254, 189, 65),
		NSColor<uint8_t>(52, 200, 74), NSColor<uint8_t>(189, 74, 70), NSColor<uint8_t>(190, 142, 48),
		NSColor<uint8_t>(39, 154, 55), NSColor<float>::UIGrayColor(), NSColor<float>::UILightGrayColor(),
		NSColor<float>::UILighterGrayColor() };
	for (unsigned int z = 0; z < sizeof(bcolors) / sizeof(NSColor<float>); z++) {
		button_colors_vbo[z] = graphics_buffer_create(sizeof(float) * BUTTON_VERTICES * 4, GRAPHICS_BUFFER_STATIC);
		NSView::BufferColor(button_colors_vbo[z], bcolors[z], BUTTON_VERTICES);
	}
	button_text_color_vbo = graphics_buffer_create(sizeof(float) * 4 * 4, GRAPHICS_BUFFER_STATIC);
	NSView::BufferColor(button_text_color_vbo, NSColor<float>::BlackColor().AlphaColor(0.5), 4);
	
	border_color_vbo = graphics_buffer_create(sizeof(float) * num_title_vertices * 4, GRAPHICS_BUFFER_STATIC);
	NSView::BufferColor(border_color_vbo, NSColor<float>::UIGrayColor(), num_title_vertices);
	
	percent(80);
	
	memset(square_vao, 0, sizeof(graphics_vertex_array_t) * 3);
	square_vao[0].bid = square_vbo;
	square_vao[0].stride = 2 * sizeof(float);
	square_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[0].usage = GRAPHICS_USAGE_POSITION;
	square_vao[1].bid = square_vbo;
	square_vao[1].stride = 2 * sizeof(float);
	square_vao[1].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[1].usage = GRAPHICS_USAGE_TEXCOORD;
	square_vao[2].bid = button_text_color_vbo;
	square_vao[2].stride = 4 * sizeof(float);
	square_vao[2].type = GRAPHICS_TYPE_FLOAT4;
	square_vao[2].usage = GRAPHICS_USAGE_COLOR;
	
	memset(button_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	button_vao[0].bid = button_vbo;
	button_vao[0].stride = 2 * sizeof(float);
	button_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	button_vao[0].usage = GRAPHICS_USAGE_POSITION;
	button_vao[1].bid = button_text_color_vbo;
	button_vao[1].stride = 4 * sizeof(float);
	button_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	button_vao[1].usage = GRAPHICS_USAGE_COLOR;

	memset(view_border_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	view_border_vao[0].bid = square_vbo;
	view_border_vao[0].stride = 2 * sizeof(float);
	view_border_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	view_border_vao[0].usage = GRAPHICS_USAGE_POSITION;
	view_border_vao[1].bid = border_color_vbo;
	view_border_vao[1].stride = 4 * sizeof(float);
	view_border_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	view_border_vao[1].usage = GRAPHICS_USAGE_COLOR;
	
	percent(100);

#undef percent
}

void Window::CreateEvent(uint8_t* data, uint32_t length) {
	NSEventWindowCreate* event = NSEventWindowCreate::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = new WSWindow;
	window->pid = event->GetPid();
	window->id = event->GetWindowID();
	window->context = event->GetGraphicsContext();
	window->SetFrame(event->GetFrame());
	window->SetTitle(event->GetTitle());
	
	delete event;
	
	uint64_t key = MakeKey(window->pid, window->id);
	windows[key] = window;
	window_list.push_back(window);
	
	Application::RegisterWindow(window->pid, window->id);
}

void Window::DestroyEvent(uint8_t* data, uint32_t length) {
	NSEventWindowDestroy* event = NSEventWindowDestroy::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = GetWindow(event->GetPid(), event->GetWindowID());
	if (!window) {
		delete event;
		return;
	}
	
	if (window->visible) {
		window->visible = false;
		Desktop::UpdateRect(window->frame);
	}
	
	if (key_window == window)
		key_window = NULL;
	
	Application::UnregisterWindow(window->pid, window->id);
	
	window_list.remove(window);
	windows.erase(MakeKey(event->GetPid(), event->GetWindowID()));
	delete window;
	
	delete event;
}

void Window::ShowEvent(uint8_t* data, uint32_t length) {
	NSEventWindowShow* event = NSEventWindowShow::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = GetWindow(event->GetPid(), event->GetWindowID());
	if (!window) {
		delete event;
		return;
	}
	
	window->visible = event->GetShow();
	
	if (event->GetAnimates()) {
		NSSize start_size = window->GetMinimumSize();
		NSRect start_frame = NSRect(NSPoint(window->frame.origin.x + window->frame.size.width / 2 - start_size.width / 2,
											window->frame.origin.y + window->frame.size.height / 2 - start_size.height / 2),
									start_size);
		NSRect end_frame = window->frame;
		window->SetFrame(start_frame);
		NSAnimation* a = new NSAnimation([window, start_frame, end_frame](const NSAnimation* anim) {
			NSPoint p = start_frame.origin + (end_frame.origin - start_frame.origin) * anim->GetValue();
			NSSize s = start_frame.size + (end_frame.size - start_frame.size) * anim->GetValue();
			window->SetFrame(NSRect(p, s));
			Desktop::UpdateRect(window->frame);
		}, [window, end_frame](const NSAnimation* anim, bool) {
			window->SetFrame(end_frame);
			Desktop::UpdateRect(window->frame);
			delete anim;
		}, 0.1);
		a->Start();
	} else {
		Desktop::UpdateRect(window->frame);
	}
	
	if (window->visible)
		MakeKeyWindow(window);
	else if (window == key_window)
		MakeKeyWindow(NULL);
	
	delete event;
}

void Window::MakeKeyEvent(uint8_t* data, uint32_t length) {
	NSEventWindowMakeKey* event = NSEventWindowMakeKey::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = GetWindow(event->GetPid(), event->GetWindowID());
	if (window) {
		if (event->GetIsKey())
			MakeKeyWindow(window);
		else if (key_window == window)
			MakeKeyWindow(NULL);
	}
	
	delete event;
}

void Window::SetTitleEvent(uint8_t* data, uint32_t length) {
	NSEventWindowSetTitle* event = NSEventWindowSetTitle::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = GetWindow(event->GetPid(), event->GetWindowID());
	if (!window) {
		delete event;
		return;
	}
	window->SetTitle(event->GetTitle());
	
	delete event;
}

void Window::SetFrameEvent(uint8_t* data, uint32_t length) {
	NSEventWindowSetFrame* event = NSEventWindowSetFrame::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = GetWindow(event->GetPid(), event->GetWindowID());
	if (!window) {
		delete event;
		return;
	}
	window->SetFrame(event->GetFrame());
	
	delete event;
}

void Window::SetButtonMaskEvent(uint8_t* data, uint32_t length) {
	NSEventWindowSetButtonMask* event = NSEventWindowSetButtonMask::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = GetWindow(event->GetPid(), event->GetWindowID());
	if (!window) {
		delete event;
		return;
	}
	window->button_mask = event->GetMask();
	Desktop::UpdateRect(window->GetButtonFrame());
	
	delete event;
}

void Window::DrawEvent(uint8_t* data, uint32_t length) {
	NSEventWindowDraw* event = NSEventWindowDraw::FromData(data, length);
	if (!event)
		return;
	
	WSWindow* window = GetWindow(event->GetPid(), event->GetWindowID());
	if (!window || !window->visible) {
		delete event;
		return;
	}
	
	std::vector<NSRect> rects = event->GetUpdateRects();
	std::vector<NSRect> draw_rects;
	draw_rects.reserve(rects.size());
	for (unsigned int z = 0; z < rects.size(); z++) {
		NSRect content_frame = window->GetContentFrame();
		NSRect out = NSRect(rects[z].origin + content_frame.origin, rects[z].size);
		if (NSRectClamp(out, content_frame, &out))
			draw_rects.emplace_back(out);
	}
	
	Desktop::UpdateRects(draw_rects);
	delete event;
}

void Window::ProcessEvent(uint8_t* data, uint32_t length) {
	uint32_t* buffer = reinterpret_cast<uint32_t*>(data);
	
	uint32_t sub_code = buffer[1];
	switch (sub_code) {
		case WINDOW_EVENT_CREATE_ID:
			CreateEvent(data, length);
			break;
		case WINDOW_EVENT_DESTROY_ID:
			DestroyEvent(data, length);
			break;
		case WINDOW_EVENT_SHOW_ID:
			ShowEvent(data, length);
			break;
		case WINDOW_EVENT_MAKE_KEY_ID:
			MakeKeyEvent(data, length);
			break;
		case WINDOW_EVENT_SET_TITLE_ID:
			SetTitleEvent(data, length);
			break;
		case WINDOW_EVENT_SET_FRAME_ID:
			SetFrameEvent(data, length);
			break;
		case WINDOW_EVENT_SET_BUTTON_MASK_ID:
			SetButtonMaskEvent(data, length);
			break;
		case WINDOW_EVENT_DRAW_ID:
			DrawEvent(data, length);
		default:
			break;
	}
}

void Window::DrawButtons(WSWindow* window, const std::vector<NSRect>& rects) {
	if (window->button_mask == 0)
		return;
	
	NSRect button_frame = window->GetButtonFrame();
	NSMatrix button_matrix = NSMatrix::Identity();
	button_matrix.Translate(button_frame.origin.x, button_frame.origin.y, 0);
	bool show_all = button_frame.ContainsPoint(Desktop::GetMousePos());
	for (int z = 0; z < NUM_BUTTONS; z++) {
		NSRect rect = NSRect(button_frame.origin.x + (BUTTON_SPACING + BUTTON_WIDTH) * z,
							 button_frame.origin.y, BUTTON_WIDTH, BUTTON_WIDTH);
		if (Desktop::RectsIntersect(rects, rect)) {
			NSMatrix m = button_matrix;
			NSMatrix border_matrix = button_matrix;
			border_matrix.Scale(BUTTON_WIDTH, BUTTON_WIDTH, 1);
			m.Translate(BUTTON_BORDER_SIZE / 2, BUTTON_BORDER_SIZE / 2, 0);
			m.Scale(BUTTON_WIDTH - BUTTON_BORDER_SIZE, BUTTON_WIDTH - BUTTON_BORDER_SIZE, 1);
			
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, border_matrix);
			button_vao[1].bid = button_colors_vbo[BUTTON_BORDER];
			graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, BUTTON_VERTICES - 2, button_vao, 2);
			
			unsigned int bmask = (1 << z);
			bool mask = (window->button_mask & bmask);
			bool show = (((window == key_window) || show_all) && mask);
			bool over = (window->button_down & bmask) && (window->button_over & bmask);
			unsigned int index = mask ? BUTTON_UNHIGHLIGHTED : BUTTON_DISABLED;
			if (show)
				index = over ? (z + NUM_BUTTONS) : z;
			button_vao[1].bid = button_colors_vbo[index];
			
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, m);
			graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, BUTTON_VERTICES - 2, button_vao, 2);
			
			if (show && (window->highlights_buttons || window->button_down != 0)) {
				// Draw symbols
				float width = BUTTON_WIDTH - BUTTON_BORDER_SIZE - BUTTON_INSET;
				float height = BUTTON_SYMBOL_HEIGHT;
				switch (bmask) {
					case BUTTON_MASK_CLOSE: {
						float angles[] = { 45, -45 };
						for (int t = 0; t < 2; t++) {
							m = button_matrix;
							m.Translate(BUTTON_WIDTH / 2, BUTTON_WIDTH / 2, 0);
							m.Rotate(0, 0, 1, angles[t]);
							m.Translate(-BUTTON_WIDTH / 2, -BUTTON_WIDTH / 2, 0);
							m.Translate((BUTTON_WIDTH - height) / 2, (BUTTON_BORDER_SIZE + BUTTON_INSET) / 2, 0);
							m.Scale(height, width, 1);
							square_vao[2].bid = button_text_color_vbo;
							graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, m);
							graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
						}
						break;
					}
					case BUTTON_MASK_MAX: {
						m = button_matrix;
						m.Translate((BUTTON_WIDTH - height) / 2, (BUTTON_BORDER_SIZE + BUTTON_INSET) / 2, 0);
						m.Scale(height, width, 1);
						square_vao[2].bid = button_text_color_vbo;
						graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, m);
						graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
					}
					case BUTTON_MASK_MIN: {
						m = button_matrix;
						m.Translate((BUTTON_BORDER_SIZE + BUTTON_INSET) / 2, (BUTTON_WIDTH - height) / 2, 0);
						m.Scale(width, height, 1);
						square_vao[2].bid = button_text_color_vbo;
						graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, m);
						graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 3);
						break;
					}
				}
			}
		}
		button_matrix.Translate(BUTTON_SPACING + BUTTON_WIDTH, 0, 0);
	}
}

void Window::Draw(const std::vector<NSRect>& rects) {
	if (rects.size() == 0)
		return;
	
	// TODO: make this iterate from farthest back to frontmost (done)
	// but then at the same time don't bother with things that are occluded
	// maybe using Window::GetVisibleRectsForWindow(WSWindow*, std::vector<NSRect>& rects)
	for (WSWindow* window : window_list) {
		if (!window->visible || !Desktop::RectsIntersect(rects, window->frame))
			continue;
		
		window->UpdateFrameIfNeeded();
		
		NSRect frame = window->frame;
		NSMatrix window_matrix = NSMatrix::Identity();
		window_matrix.Translate(frame.origin.x, frame.origin.y, 0);
		
		NSRect title_frame = window->GetTitleFrame();
		if (Desktop::RectsIntersect(rects, title_frame)) {
			// Draw border
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, window_matrix);
			uint32_t backup = window->title_vao[1].bid;
			window->title_vao[1].bid = border_color_vbo;
			graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, num_title_vertices - 2, window->title_vao, 2);
			
			NSMatrix title_matrix = window_matrix;
			title_matrix.Translate(1, 1, 0);
			title_matrix.Scale(1 - (2 / title_frame.size.width), 1 - (1 / title_frame.size.height), 1);
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, title_matrix);
			window->title_vao[1].bid = backup;
			graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, num_title_vertices - 2, window->title_vao, 2);
		}
		
		DrawButtons(window, rects);
		
		NSRect text_frame = NSRect(frame.origin.x + (frame.size.width - window->text_size.width) / 2,
								   frame.origin.y + (WINDOW_TITLE_BAR_HEIGHT - window->text_height) / 2,
								   window->text_size.width, window->text_size.height);
		if (text_frame.origin.x - frame.origin.x < TOTAL_BUTTON_SIZE)
			text_frame.origin.x = frame.origin.x + TOTAL_BUTTON_SIZE;
		if (Desktop::RectsIntersect(rects, text_frame)) {
			NSMatrix text_matrix = NSMatrix::Identity();
			text_matrix.Translate(text_frame.origin.x, text_frame.origin.y, 0);
			text_matrix.Scale(text_frame.size.width, text_frame.size.height, 1);
			graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, window->text_img);
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, text_matrix);
			graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 2);
		}
				
		NSRect content_frame = window->GetContentFrame();
		if (Desktop::RectsIntersect(rects, content_frame)) {
			NSMatrix matrix = NSMatrix::Identity();
			matrix.Translate(content_frame.origin.x, content_frame.origin.y, 0);
			matrix.Scale(content_frame.size.width, content_frame.size.height, 1);
			graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE,
									   window->context.color_surface);
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
			graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 2);
		}
		
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
		
		const NSRect border_rects[4] = {
			NSRect(content_frame.origin.x, content_frame.origin.y, content_frame.size.width, 1),
			NSRect(content_frame.origin.x, content_frame.origin.y + content_frame.size.height - 1, content_frame.size.width, 1),
			NSRect(content_frame.origin.x, content_frame.origin.y, 1, content_frame.size.height),
			NSRect(content_frame.origin.x + content_frame.size.width - 1, content_frame.origin.y, 1, content_frame.size.height),
		};
		for (unsigned int z = 0; z < sizeof(border_rects) / sizeof(NSRect); z++) {
			const NSRect& border_rect = border_rects[z];
			if (Desktop::RectsIntersect(rects, border_rect)) {
				NSMatrix border_matrix = NSMatrix::Identity();
				border_matrix.Translate(border_rect.origin.x, border_rect.origin.y, 0);
				border_matrix.Scale(border_rect.size.width, border_rect.size.height, 1);
				graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, border_matrix);
				graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, view_border_vao, 2);
			}
		}
	}
}

void Window::MakeKeyWindow(uint32_t pid, uint32_t wid) {
	MakeKeyWindow(GetWindow(pid, wid));
}

void Window::MakeKeyWindow(WSWindow* key) {
	if (key_window == key && key != NULL) {
		if (window_list.back() != key) {
			// Move to front
			window_list.remove(key);
			window_list.push_back(key);
			Desktop::UpdateRect(key_window->frame);
		}
		return;
	}
	
	if (key_window) {
		key_window->title_vao[1].bid = title_colors_vbo;
		Desktop::UpdateRect(key_window->GetTitleFrame());
		
		auto e = NSEventWindowMakeKey(key_window->pid, key_window->id, false);
		Application::SendEvent(&e, key_window->pid);
	}
	key_window = key;
	if (key) {
		key_window->cursor = NSCursor::CURSOR_DEFAULT;
		key_window->title_vao[1].bid = title_key_colors_vbo;
		
		// Move to front
		window_list.remove(key);
		window_list.push_back(key);
		
		Application::SetActiveApplication(key->pid);
		
		auto e = NSEventWindowMakeKey(key_window->pid, key_window->id, true);
		Application::SendEvent(&e, key_window->pid);
		
		Desktop::UpdateRect(key_window->frame);
	}
}

bool Window::IsVisible(uint32_t pid, uint32_t wid) {
	WSWindow* window = GetWindow(pid, wid);
	if (!window)
		return NULL;
	return window->visible;
}

uint32_t Window::FindLastVisibleWindow(uint32_t pid) {
	for (auto it = window_list.rbegin(); it != window_list.rend(); it++) {
		if ((*it)->pid == pid && (*it)->visible)
			return (*it)->id;
	}
	return -1;
}

uint32_t Window::FindLastVisiblePid(uint32_t not_pid) {
	for (auto it = window_list.rbegin(); it != window_list.rend(); it++) {
		WSWindow* window = *it;
		if (window->visible && window->pid != not_pid)
			return window->pid;
	}
	return 0;
}

void Window::DestroyWindows(uint32_t pid) {
	auto it = window_list.begin();
	while (it != window_list.end()) {
		WSWindow* window = *it;
		if (window->pid == pid) {
			it = window_list.erase(it);
			windows.erase(MakeKey(window->pid, window->id));
			if (window->visible)
				Desktop::UpdateRect(window->frame);
			delete window;
		} else
			it++;
	}
}

int Window::GetCornersForFrame(const NSRect& frame, const NSPoint& p) {
	NSRect left_frame = NSRect(frame.origin.x - 3, frame.origin.y, 6, frame.size.height);
	NSRect right_frame = NSRect(frame.origin.x + frame.size.width - 3, frame.origin.y, 6, frame.size.height);
	NSRect top_frame = NSRect(frame.origin.x, frame.origin.y - 3, frame.size.width, 6);
	NSRect bottom_frame = NSRect(frame.origin.x, frame.origin.y + frame.size.height - 3, frame.size.width, 6);
	
	NSRect top_left = NSRect(frame.origin.x, frame.origin.y, 10, 10);
	NSRect top_right = NSRect(frame.origin.x + frame.size.width - 10, frame.origin.y, 10, 10);
	NSRect bottom_left = NSRect(frame.origin.x, frame.origin.y + frame.size.height - 10, 10, 10);
	NSRect bottom_right = NSRect(frame.origin.x + frame.size.width - 10, frame.origin.y + frame.size.height - 10, 10, 10);
	
	int ret = 0;
	if (left_frame.ContainsPoint(p))
		ret |= WINDOW_CORNER_LEFT;
	if (right_frame.ContainsPoint(p))
		ret |= WINDOW_CORNER_RIGHT;
	if (top_frame.ContainsPoint(p))
		ret |= WINDOW_CORNER_TOP;
	if (bottom_frame.ContainsPoint(p))
		ret |= WINDOW_CORNER_BOTTOM;
	if (top_left.ContainsPoint(p))
		ret |= WINDOW_CORNER_LEFT | WINDOW_CORNER_TOP;
	if (top_right.ContainsPoint(p))
		ret |= WINDOW_CORNER_RIGHT | WINDOW_CORNER_TOP;
	if (bottom_left.ContainsPoint(p))
		ret |= WINDOW_CORNER_LEFT | WINDOW_CORNER_BOTTOM;
	if (bottom_right.ContainsPoint(p))
		ret |= WINDOW_CORNER_RIGHT | WINDOW_CORNER_BOTTOM;
	
	return ret;
}

NSCursor::Cursor Window::GetCursorForFrame(const NSRect& frame, const NSPoint& p) {
	NSCursor::Cursor cursor = NSCursor::CURSOR_DEFAULT;
	
	int corners = GetCornersForFrame(frame, p);
	
	bool over_left = (corners & WINDOW_CORNER_LEFT) != 0;
	bool over_right = (corners & WINDOW_CORNER_RIGHT) != 0;
	bool over_top = (corners & WINDOW_CORNER_TOP) != 0;
	bool over_bottom = (corners & WINDOW_CORNER_BOTTOM) != 0;
	
	if ((over_left && over_top) || (over_right && over_bottom))
		cursor = NSCursor::CURSOR_RESIZE_NORTHWEST_SOUTHEAST;
	else if ((over_left && over_bottom) || (over_right && over_top))
		cursor = NSCursor::CURSOR_RESIZE_NORTHEAST_SOUTHWEST;
	else if (over_top || over_bottom)
		cursor = NSCursor::CURSOR_RESIZE_NORTH_SOUTH;
	else if (over_left || over_right)
		cursor = NSCursor::CURSOR_RESIZE_EAST_WEST;
	
	return cursor;
}

void Window::UpdateFrame(WSWindow* window, NSRect frame, bool delayed) {
	window->SetFrame(frame);
	
	if (!delayed) {
		auto e = NSEventWindowSetFrame(window->pid, window->id, window->frame);
		Application::SendEvent(&e, window->pid);
	}
}

bool Window::MouseDown(NSPoint p, NSMouseButton mouse, uint32_t click_count) {
	bool found = false;
	bool process = true;
	for (auto it = window_list.rbegin(); it != window_list.rend(); it++) {
		WSWindow* window = *it;
		if (!window->visible)
			continue;
		NSRect title_bar = window->GetTitleFrame();
		NSRect content_frame = window->GetContentFrame();
		if (title_bar.ContainsPoint(p)) {
			down_title = true;
			
			// Check for button presses
			for (int z = 0; z < NUM_BUTTONS; z++) {
				if (!(window->button_mask & (1 << z)))
					continue;
				NSRect button_rect = window->GetButtonFrame(z);
				if (button_rect.ContainsPoint(p)) {
					window->SetButtonDown(z, true);
					down_title = false;
					process = false;
					found = true;
					break;
				}
			}
		} else if (!content_frame.ContainsPoint(p))
			continue;
		
		if (key_window == window && !down_title) {
			NSEventMouse* event = NSEventMouse::Create(p - content_frame.origin,
													   NSMouseTypeDown, mouse, click_count, window->id);
			Application::SendEvent(event, window->pid);
			delete event;
		}
		
		if (mouse == NSMouseButtonLeft)
			MakeKeyWindow(window);
		
		found = true;
		break;
	}
	
	if (process) {
		if (key_window && mouse == NSMouseButtonLeft) {
			int corners = GetCornersForFrame(key_window->frame, p);
			if (corners != 0) {
				Desktop::SetCursor(GetCursorForFrame(key_window->frame, p));
				key_window->resizing = true;
				key_window->resizing_corners = corners;
				found = true;
				down_title = false;
			}
		}
	}
	
	if (found) {
		mouse_down = true;
		mouse_pos = p;
		mouse_down_type = mouse;
	}
	
	return found;
}

bool Window::MouseMoved(NSPoint p) {
	// Check all windows for intersecting button frame
	for (auto it = window_list.rbegin(); it != window_list.rend(); it++) {
		WSWindow* window = *it;
		if (!window->visible || !window->button_mask)
			continue;
		window->SetHighlightsButtons(window->GetButtonFrame().ContainsPoint(p));
		bool over = false;
		for (int z = 0; z < NUM_BUTTONS; z++) {
			if (!(window->button_mask & (1 << z)) || !(window->button_down & (1 << z)))
				continue;
			bool o = window->GetButtonFrame(z).ContainsPoint(p);
			window->SetButtonOver(z, o);
			over |= o;
		}
		if (over)
			return true;
	}
	
	if (!mouse_down) {
		bool over = false;
		if (key_window && key_window->visible) {
			NSRect frame = key_window->frame;
			NSCursor::Cursor cursor = GetCursorForFrame(frame, p);
			
			if (cursor != key_window->cursor) {
				Desktop::SetCursor(cursor);
				key_window->cursor = cursor;
			}
			over = frame.ContainsPoint(p) || (cursor != NSCursor::CURSOR_DEFAULT);
		}
		if (!over)
			Desktop::SetCursor(NSCursor::CURSOR_DEFAULT);
		
		return over;
	}
	
	if (mouse_down_type == NSMouseButtonLeft && down_title)
		Window::UpdateFrame(key_window, NSRect(key_window->frame.origin + (p - mouse_pos), key_window->frame.size));
	else if (mouse_down_type == NSMouseButtonLeft && key_window && key_window->resizing) {
		NSRect frame = key_window->frame;
		
		NSPoint delta = p - mouse_pos;
		int corners = key_window->resizing_corners;
		NSSize min_size = key_window->GetMinimumSize();
		if (corners & WINDOW_CORNER_LEFT) {
			frame.origin.x += delta.x;
			frame.size.width -= delta.x;
			if (frame.size.width < min_size.width) {
				frame.origin.x -= (min_size.width - frame.size.width);
				frame.size.width = min_size.width;
			}
		}
		if (corners & WINDOW_CORNER_TOP) {
			frame.origin.y += delta.y;
			frame.size.height -= delta.y;
			if (frame.size.height < min_size.height) {
				frame.origin.y -= (min_size.height - frame.size.height);
				frame.size.height = min_size.height;
			}
		}
		if (corners & WINDOW_CORNER_RIGHT)
			frame.size.width += delta.x;
		if (corners & WINDOW_CORNER_BOTTOM)
			frame.size.height += delta.y;
		
		Window::UpdateFrame(key_window, frame, true);
	}
	
	mouse_pos = p;
	
	return true;
}

bool Window::MouseUp(NSPoint p, NSMouseButton mouse) {
	if (!mouse_down) {
		if (mouse == NSMouseButtonLeft)
			MakeKeyWindow(NULL);
		return false;
	}
	
	// Check if we have any window button events
	bool process = true;
	for (auto it = window_list.rbegin(); it != window_list.rend(); it++) {
		WSWindow* window = *it;
		if (!window->visible || !window->button_mask)
			continue;
		
		if ((window->button_down & BUTTON_MASK_CLOSE) && (window->button_over & BUTTON_MASK_CLOSE)) {
			// Close was pressed
			process = false;
			
			auto event = NSEventWindowShow(window->pid, window->id, false, false);
			Application::SendEvent(&event, window->pid);
		}
		for (int z = 0; z < NUM_BUTTONS; z++) {
			window->SetButtonDown(z, false);
			window->SetButtonOver(z, false);
		}
		window->SetHighlightsButtons(window->GetButtonFrame().ContainsPoint(p));
	}
	
	if (key_window && process) {
		if (mouse == NSMouseButtonLeft) {
			if (key_window->resizing) {
				key_window->resizing = false;
				Window::UpdateFrame(key_window, key_window->frame);
			}
		}
		
		if (!down_title) {
			NSEventMouse* event = NSEventMouse::Create(p - key_window->GetContentFrame().origin,
													   NSMouseTypeUp, mouse, 0, key_window->id);
			Application::SendEvent(event, key_window->pid);
			delete event;
		}
	}
	
	if (mouse == NSMouseButtonLeft) {
		mouse_down = false;
		down_title = false;
	}
	
	return true;
}

bool Window::MouseScrolled(NSPoint p, float delta_x, float delta_y) {
	for (auto it = window_list.rbegin(); it != window_list.rend(); it++) {
		WSWindow* window = *it;
		if (!window->visible)
			continue;
		NSRect content_frame = window->GetContentFrame();
		if (!content_frame.ContainsPoint(p))
			continue;
		
		NSEventMouse* event = NSEventMouse::Create(p - content_frame.origin,
													NSMouseTypeScrolled, NSMouseButtonNone, 0, window->id);
		event->SetDeltaX(delta_x);
		event->SetDeltaY(delta_y);
		Application::SendEvent(event, window->pid);
		delete event;
		
		return true;
	}
	
	return false;
}

bool Window::KeyDown(unsigned char key, NSModifierFlags flags) {
	if (key_window) {
		NSEventKey* event = NSEventKey::Create(key, true, flags, key_window->id);
		Application::SendEvent(event, key_window->pid);
		delete event;
		
		return true;
	}
	return false;
}

bool Window::KeyUp(unsigned char key, NSModifierFlags flags) {
	if (key_window) {
		NSEventKey* event = NSEventKey::Create(key, false, flags, key_window->id);
		Application::SendEvent(event, key_window->pid);
		delete event;
		
		return true;
	}
	return false;
}
