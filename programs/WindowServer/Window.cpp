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

using std::string;

class WSWindow;

namespace Window {
	std::unordered_map<uint64_t, WSWindow*> windows;
	std::list<WSWindow*> window_list;
	
	constexpr float round_width = 5.0;
	constexpr int num_title_vertices = 22;	// must be multiple of 2 + 4
	constexpr float button_width = 12;
	constexpr float button_spacing = 6;
	constexpr float button_offset = 10;
	
	uint32_t title_key_colors_vbo;
	uint32_t title_colors_vbo;
	uint32_t border_color_vbo;
	uint32_t button_vbo;
	uint32_t square_vbo;
	uint32_t border_vbo;
	uint32_t button_images[3];
	graphics_vertex_array_t button_vao[6];
	graphics_vertex_array_t square_vao[2];
	graphics_vertex_array_t view_border_vao[2];
	
	// Events
	void CreateEvent(uint8_t* data, uint32_t length);
	void DestroyEvent(uint8_t* data, uint32_t length);
	void ShowEvent(uint8_t* data, uint32_t length);
	void MakeKeyEvent(uint8_t* data, uint32_t length);
	void SetTitleEvent(uint8_t* data, uint32_t length);
	void SetFrameEvent(uint8_t* data, uint32_t length);
	void DrawEvent(uint8_t* data, uint32_t length);
	
	// Mouse
	NSPoint mouse_pos;
	bool mouse_down = false;
	NSMouseButton mouse_down_type = NSMouseButtonNone;
	bool down_title = false;
	
	// Key window
	void MakeKeyWindow(WSWindow* key);
	WSWindow* key_window = NULL;
	
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
	uint32_t title_border_vbo;
	uint32_t title_border_colors_vbo;
	graphics_vertex_array_t title_border_vao[2];
	uint32_t text_img;
	NSSize text_size;
	float text_height;
	
	bool visible;
	
	inline NSRect GetTitleFrame() const {
		return NSRect(frame.origin.x, frame.origin.y, frame.size.width, WINDOW_TITLE_BAR_HEIGHT);
	}
	inline NSRect GetContentFrame() const {
		return NSRect(frame.origin.x, frame.origin.y + WINDOW_TITLE_BAR_HEIGHT, frame.size.width,
					  frame.size.height - WINDOW_TITLE_BAR_HEIGHT);
	}
	
	WSWindow() {
		using namespace Window;
		
		memset(&context, 0, sizeof(context));
		memset(title_vao, 0, sizeof(title_vao));
		memset(title_border_vao, 0, sizeof(title_border_vao));
		visible = false;
		
		title_vao[0].bid = title_vbo;
		title_vao[0].offset = 0;
		title_vao[0].stride = 2 * sizeof(float);
		title_vao[0].type = GRAPHICS_TYPE_FLOAT2;
		title_vao[0].usage = GRAPHICS_USAGE_POSITION;
		title_vao[1].bid = title_colors_vbo;
		title_vao[1].offset = 0;
		title_vao[1].stride = 3 * sizeof(float);
		title_vao[1].type = GRAPHICS_TYPE_FLOAT3;
		title_vao[1].usage = GRAPHICS_USAGE_COLOR;
		
		NSColor<float> c = NSColor<float>::UIGrayColor();
		float line_colors[(num_title_vertices - 1) * 3];
		for (int z = 0; z < (num_title_vertices - 1); z++) {
			line_colors[z * 3 + 0] = c.r;
			line_colors[z * 3 + 1] = c.g;
			line_colors[z * 3 + 2] = c.b;
		}
		title_border_colors_vbo = graphics_buffer_create(sizeof(line_colors), GRAPHICS_BUFFER_STATIC);
		graphics_buffer_data(title_border_colors_vbo, line_colors, sizeof(line_colors));
		title_border_vao[0].bid = title_border_vbo;
		title_border_vao[0].offset = 0;
		title_border_vao[0].stride = 2 * sizeof(float);
		title_border_vao[0].type = GRAPHICS_TYPE_FLOAT2;
		title_border_vao[0].usage = GRAPHICS_USAGE_POSITION;
		title_border_vao[1].bid = title_border_colors_vbo;
		title_border_vao[1].offset = 0;
		title_border_vao[1].stride = 3 * sizeof(float);
		title_border_vao[1].type = GRAPHICS_TYPE_FLOAT3;
		title_border_vao[1].usage = GRAPHICS_USAGE_COLOR;
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
		if (title_border_vbo) {
			graphics_buffer_destroy(title_border_vbo);
			title_border_vbo = 0;
		}
		if (title_border_colors_vbo) {
			graphics_buffer_destroy(title_border_colors_vbo);
			title_border_colors_vbo = 0;
		}
	}
	
	void SetTitle(string t) {
		title = t;
		NSFont font;
		NSImage* text = font.GetImage(title, NSColor<float>::BlackColor());
		text_size = text->GetScaledSize();
		text_height = font.GetLineHeight();
		int w = int(text->GetSize().width + 0.5f);
		int h = int(text->GetSize().height + 0.5f);
		if (text_img)
			graphics_buffer_destroy(text_img);
		text_img = graphics_buffer_create(w, h, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
		graphics_buffer_data(text_img, text->GetPixelData(), w, h, w * h * 4);
		delete text;
		
		if (visible)
			Desktop::UpdateRect(GetTitleFrame());
	}
	
	void SetFrame(NSRect f) {
		using namespace Window;
		
		NSRect old_frame = frame;
		frame = f;
		
		float vertices[num_title_vertices * 2];
		float border_verts[(num_title_vertices - 1) * 2];
		vertices[0] = frame.size.width / 2;
		vertices[1] = WINDOW_TITLE_BAR_HEIGHT / 2;
		uint32_t pos = 2;
		for (int z = 0; z < (num_title_vertices - 4) / 2; z++) {
			float angle = (180.0f - (float(z) / (((num_title_vertices - 4) / 2) - 1)) * 90.0f) / 180.0f * M_PI;
			float x = cosf(angle) * round_width + round_width;
			float y = -sinf(angle) * round_width + round_width;
			border_verts[pos - 2] = x;
			border_verts[pos - 1] = y;
			vertices[pos++] = x;
			vertices[pos++] = y;
		}
		for (int z = 0; z < (num_title_vertices - 4) / 2; z++) {
			float angle = (90.0f - (float(z) / (((num_title_vertices - 4) / 2) - 1)) * 90.0f) / 180.0f * M_PI;
			float x = cosf(angle) * round_width + frame.size.width - round_width;
			float y = -sinf(angle) * round_width + round_width;
			border_verts[pos - 2] = x;
			border_verts[pos - 1] = y;
			vertices[pos++] = x;
			vertices[pos++] = y;
		}
		border_verts[pos - 2] = frame.size.width;
		border_verts[pos - 1] = WINDOW_TITLE_BAR_HEIGHT;
		vertices[pos++] = frame.size.width;
		vertices[pos++] = WINDOW_TITLE_BAR_HEIGHT;
		border_verts[pos - 2] = 0;
		border_verts[pos - 1] = WINDOW_TITLE_BAR_HEIGHT;
		vertices[pos++] = 0;
		vertices[pos++] = WINDOW_TITLE_BAR_HEIGHT;
		border_verts[pos - 2] = 0;
		border_verts[pos - 1] = round_width;
		vertices[pos++] = 0;
		vertices[pos++] = round_width;
		
		if (title_vbo)
			graphics_buffer_destroy(title_vbo);
		title_vbo = graphics_buffer_create(sizeof(vertices), GRAPHICS_BUFFER_STATIC);
		graphics_buffer_data(title_vbo, vertices, sizeof(vertices));
		title_vao[0].bid = title_vbo;
		
		if (title_border_vbo)
			graphics_buffer_destroy(title_border_vbo);
		title_border_vbo = graphics_buffer_create(sizeof(border_verts), GRAPHICS_BUFFER_STATIC);
		graphics_buffer_data(title_border_vbo, border_verts, sizeof(border_verts));
		title_border_vao[0].bid = title_border_vbo;
		
		if (visible) {
			Desktop::UpdateRect(old_frame);
			Desktop::UpdateRect(frame);
		}
	}
};

void Window::Load(volatile float* p, float percent_start, float percent_end) {
#define percent(x) *p = percent_start + (percent_end - percent_start) / 100.0f * x
	
	percent(0);
	
	float title_colors[num_title_vertices * 3];
	NSColor<float> c = NSColor<float>::UILightGrayColor();
	for (int z = 0; z < num_title_vertices; z++) {
		title_colors[z * 3 + 0] = c.r;
		title_colors[z * 3 + 1] = c.g;
		title_colors[z * 3 + 2] = c.b;
	}
	title_key_colors_vbo = graphics_buffer_create(sizeof(title_colors), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(title_key_colors_vbo, title_colors, sizeof(title_colors));
	c = NSColor<float>::UILighterGrayColor();
	for (int z = 0; z < num_title_vertices; z++) {
		title_colors[z * 3 + 0] = c.r;
		title_colors[z * 3 + 1] = c.g;
		title_colors[z * 3 + 2] = c.b;
	}
	title_colors_vbo = graphics_buffer_create(sizeof(title_colors), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(title_colors_vbo, title_colors, sizeof(title_colors));
	
	percent(20);
	
	const float button_verts[] = {
		0, 0,
		button_width, 0,
		0, button_width,
		button_width, button_width
	};
	const float square[] = {
		0, 0,
		1.0f, 0,
		0, 1.0f,
		1.0f, 1.0f
	};
	button_vbo = graphics_buffer_create(sizeof(button_verts), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(button_vbo, button_verts, sizeof(button_verts));
	square_vbo = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(square_vbo, square, sizeof(square));
	
	percent(40);
	
	const char* button_imgs[] = { "/system/images/button_close.png",
		"/system/images/button_minimize.png", "/system/images/button_maximize.png" };
	memset(button_vao, 0, sizeof(graphics_vertex_array_t) * 6);
	for (int z = 0; z < 3; z++) {
		NSImage* img = new NSImage(button_imgs[z]);
		int img_width = int(img->GetSize().width + 0.5f);
		int img_height = int(img->GetSize().height + 0.5f);
		button_images[z] = graphics_buffer_create(img_width, img_height, GRAPHICS_BUFFER_STATIC,
												  GRAPHICS_FORMAT_A8R8G8B8);
		graphics_buffer_data(button_images[z], img->GetPixelData(), img_width, img_height,
							 img_width * img_height * sizeof(uint32_t));
		delete img;
		
		button_vao[z*2].bid = button_vbo;
		button_vao[z*2].offset = 0;
		button_vao[z*2].stride = 2 * sizeof(float);
		button_vao[z*2].type = GRAPHICS_TYPE_FLOAT2;
		button_vao[z*2].usage = GRAPHICS_USAGE_POSITION;
		button_vao[z*2+1].bid = square_vbo;
		button_vao[z*2+1].offset = 0;
		button_vao[z*2+1].stride = 2 * sizeof(float);
		button_vao[z*2+1].type = GRAPHICS_TYPE_FLOAT2;
		button_vao[z*2+1].usage = GRAPHICS_USAGE_TEXCOORD;
		
		percent(50 + 20 * z);
	}
	
	memset(square_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	square_vao[0].bid = square_vbo;
	square_vao[0].offset = 0;
	square_vao[0].stride = 2 * sizeof(float);
	square_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[0].usage = GRAPHICS_USAGE_POSITION;
	square_vao[1].bid = square_vbo;
	square_vao[1].offset = 0;
	square_vao[1].stride = 2 * sizeof(float);
	square_vao[1].type = GRAPHICS_TYPE_FLOAT2;
	square_vao[1].usage = GRAPHICS_USAGE_TEXCOORD;
	
	const float border_verts[] = {
		0, 0,
		1, 0,
		1, 1,
		0, 1,
		0, 0
	};
	border_vbo = graphics_buffer_create(sizeof(border_verts), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(border_vbo, border_verts, sizeof(border_verts));
	
	c = NSColor<float>::UIGrayColor();
	float line_colors[5*3];
	for (int z = 0; z < 5; z++) {
		line_colors[z * 3 + 0] = c.r;
		line_colors[z * 3 + 1] = c.g;
		line_colors[z * 3 + 2] = c.b;
	}
	border_color_vbo = graphics_buffer_create(sizeof(line_colors), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(border_color_vbo, line_colors, sizeof(line_colors));
	
	memset(view_border_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	view_border_vao[0].bid = border_vbo;
	view_border_vao[0].offset = 0;
	view_border_vao[0].stride = 2 * sizeof(float);
	view_border_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	view_border_vao[0].usage = GRAPHICS_USAGE_POSITION;
	view_border_vao[1].bid = border_color_vbo;
	view_border_vao[1].offset = 0;
	view_border_vao[1].stride = 3 * sizeof(float);
	view_border_vao[1].type = GRAPHICS_TYPE_FLOAT3;
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
	
	if (key_window == window) {
		// TODO: go to the next highest window?
		key_window = NULL;
	}
	
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
	if (window->visible)
		MakeKeyWindow(window);
	else
		MakeKeyWindow(NULL);
	Desktop::UpdateRect(window->frame);
	
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
		case WINDOW_EVENT_DRAW_ID:
			DrawEvent(data, length);
		default:
			break;
	}
}

void Window::Draw(const std::vector<NSRect>& rects) {
	if (rects.size() == 0)
		return;
	
	// TODO: make this iterate from farthest back to frontmost
	// but then at the same time don't bother with things that are occluded
	for (WSWindow* window : window_list) {
		if (!window->visible || !Desktop::RectsIntersect(rects, window->frame))
			continue;
		
		NSRect frame = window->frame;
		NSMatrix window_matrix = NSMatrix::Identity();
		window_matrix.Translate(frame.origin.x, frame.origin.y, 0);
		
		if (Desktop::RectsIntersect(rects, window->GetTitleFrame())) {
			NSMatrix title_matrix = window_matrix;
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, title_matrix);
			graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLEFAN, num_title_vertices - 2, window->title_vao, 2);
			graphics_draw(&context, GRAPHICS_PRIMITIVE_LINESTRIP, num_title_vertices - 2,
						  window->title_border_vao, 2);
		}
		
		NSMatrix button_matrix = window_matrix;
		button_matrix.Translate(button_offset, (WINDOW_TITLE_BAR_HEIGHT - button_width) / 2, 0);
		for (int z = 0; z < 3; z++) {
			NSRect rect = NSRect(frame.origin.x + button_offset + (button_spacing + button_width) * z,
								 frame.origin.y + (WINDOW_TITLE_BAR_HEIGHT - button_width) / 2,
								 button_width, button_width);
			if (Desktop::RectsIntersect(rects, rect)) {
				graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, button_images[z]);
				graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, button_matrix);
				graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, &button_vao[z * 2], 2);
			}
			button_matrix.Translate(button_spacing + button_width, 0, 0);
		}
		
		NSRect text_frame = NSRect(frame.origin.x + (frame.size.width - window->text_size.width) / 2,
								   frame.origin.y + (WINDOW_TITLE_BAR_HEIGHT - window->text_height) / 2,
								   window->text_size.width, window->text_size.height);
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
			// TODO: make this not copy redundant pixels?
			for (uint32_t z = 0; z < rects.size(); z++) {
				NSRect intersect = rects[z].Intersection(content_frame);
				if (intersect.size.width < 0.5f || intersect.size.height < 0.5f)
					continue;
				/*graphics_buffer_copy(context.color_surface, window->context.color_surface, intersect.origin.x,
									 intersect.origin.y, intersect.origin.x - content_frame.origin.x,
									 intersect.origin.y - content_frame.origin.y, intersect.size.width,
									 intersect.size.height);*/
				NSMatrix matrix = NSMatrix::Identity();
				matrix.Translate(content_frame.origin.x, content_frame.origin.y, 0);
				matrix.Scale(content_frame.size.width, content_frame.size.height, 1);
				graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE,
										   window->context.color_surface);
				graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
				graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, square_vao, 2);

			}
		}
		
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
		
		// Draw border
		NSRect border_rect = NSRect(frame.origin.x, frame.origin.y + WINDOW_TITLE_BAR_HEIGHT,
									frame.size.width, frame.size.height - WINDOW_TITLE_BAR_HEIGHT);
		if (Desktop::RectsIntersect(rects, border_rect)) {
			NSMatrix border_matrix = NSMatrix::Identity();
			border_matrix.Translate(border_rect.origin.x, border_rect.origin.y, 0);
			border_matrix.Scale(border_rect.size.width, border_rect.size.height, 1);
			graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, border_matrix);
			graphics_draw(&context, GRAPHICS_PRIMITIVE_LINESTRIP, 4, view_border_vao, 2);
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

bool Window::MouseDown(NSPoint p, NSMouseButton mouse) {
	for (auto it = window_list.rbegin(); it != window_list.rend(); it++) {
		WSWindow* window = *it;
		if (!window->visible)
			continue;
		NSRect title_bar = window->GetTitleFrame();
		NSRect content_frame = window->GetContentFrame();
		if (title_bar.ContainsPoint(p)) {
			down_title = true;
		} else if (!content_frame.ContainsPoint(p))
			continue;
		mouse_down = true;
		mouse_pos = p;
		mouse_down_type = mouse;
		
		if (key_window == window && !down_title) {
			NSEventMouse* event = NSEventMouse::Create(mouse_pos - content_frame.origin,
													   NSMouseTypeDown, mouse, window->id);
			Application::SendEvent(event, window->pid);
			delete event;
		}
		
		if (mouse == NSMouseButtonLeft)
			MakeKeyWindow(window);
		return true;
	}
	
	return false;
}

bool Window::MouseMoved(NSPoint p) {
	if (!mouse_down)
		return false;
	
	if (mouse_down_type == NSMouseButtonLeft && down_title) {
		NSRect old_rect = key_window->frame;
		key_window->frame.origin += (p - mouse_pos);
	
		auto e = NSEventWindowSetFrame(key_window->pid, key_window->id, key_window->frame);
		Application::SendEvent(&e, key_window->pid);
		
		std::vector<NSRect> rects = { old_rect, key_window->frame };
		Desktop::UpdateRects(rects);
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
	
	if (key_window) {
		NSEventMouse* event = NSEventMouse::Create(p - key_window->GetContentFrame().origin,
												   NSMouseTypeUp, mouse, key_window->id);
		Application::SendEvent(event, key_window->pid);
		delete event;
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
													NSMouseTypeScrolled, NSMouseButtonNone, window->id);
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
