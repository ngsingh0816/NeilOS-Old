//
//  Boot.cpp
//  product
//
//  Created by Neil Singh on 2/10/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "Boot.h"

#include "Desktop.h"
#include "Events/NSEventDefs.cpp"
#include "Window.h"

#include <graphics/graphics.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

namespace Boot {
	pthread_t loading_thread;
	volatile float percent = 0.0f;
	volatile float current_percent = 0.0f;
	NSTimer* timer;
	
	const float square[] = {
		0, 0,
		1, 0,
		0, 1,
		1, 1
	};
	const float color[] = {
		0.7, 0.7, 0.7, 1,
		0.7, 0.7, 0.7, 1,
		0.7, 0.7, 0.7, 1,
		0.7, 0.7, 0.7, 1,
	};
	const float white[] = {
		1, 1, 1, 1,
		1, 1, 1, 1,
		1, 1, 1, 1,
		1, 1, 1, 1,
	};
	
	uint32_t square_vbo;
	uint32_t color_vbo;
	uint32_t white_vbo;
	graphics_vertex_array_t vao[2];
	graphics_vertex_array_t grayvao[2];
	volatile bool drawing;
	
	void* ThreadFunction(void*);
	void Draw(NSTimer*);
}

void* Boot::ThreadFunction(void*) {
	Desktop::Load(&percent, 0, 95);
	Window::Load(&percent, 95, 100);
	
	// Wait for drawing to catch up
	while (current_percent < 99.5f)
		sched_yield();
	
	timer->Invalidate();
	
	// Cleanup
	while (drawing) {}
	graphics_buffer_destroy(square_vbo);
	graphics_buffer_destroy(color_vbo);
	graphics_buffer_destroy(white_vbo);
	
	NSHandler(Desktop::Start).Post();
	
	return NULL;
}

void Boot::Draw(NSTimer* draw_timer) {
	drawing = true;
	
	float res_x = graphics_info.resolution_x;
	float res_y = graphics_info.resolution_y;
	
	constexpr float width = 200;
	constexpr float height = 15;
	constexpr float line_width = 3;
	
	graphics_clear(&context, 0x0, 1.0f, 0, GRAPHICS_CLEAR_COLOR, 0, 0,
				   graphics_info.resolution_x, graphics_info.resolution_y);
	
	// Left
	NSMatrix matrix = NSMatrix::Identity();
	matrix.Translate((res_x - width) / 2, (res_y - height) / 2, 0);
	matrix.Scale(line_width, height + line_width, 1);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, grayvao, 2);
	
	// Right
	matrix = NSMatrix::Identity();
	matrix.Translate((res_x + width) / 2, (res_y - height) / 2, 0);
	matrix.Scale(line_width, height + line_width, 1);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, grayvao, 2);
	
	// Top
	matrix = NSMatrix::Identity();
	matrix.Translate((res_x - width) / 2, (res_y - height) / 2, 0);
	matrix.Scale(width, line_width, 1);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, grayvao, 2);
	
	// Bottom
	matrix = NSMatrix::Identity();
	matrix.Translate((res_x - width) / 2, (res_y + height) / 2, 0);
	matrix.Scale(width, line_width, 1);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, grayvao, 2);
	
	if (fabs(current_percent - percent) > 1) {
		current_percent += (percent - current_percent) / 5.0f;
	} else
		current_percent = percent;
	if (current_percent > 100)
		current_percent = 100;
	
	matrix = NSMatrix::Identity();
	matrix.Translate((res_x - width) / 2 + line_width, (res_y - height) / 2 + line_width, 0);
	matrix.Scale((width - line_width) * current_percent / 100.0f, height - line_width, 1);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, matrix);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLESTRIP, 2, vao, 2);
	
	NSRect rect = NSRect((res_x - width) / 2 - line_width, (res_y - height) / 2 - line_width,
						 width + line_width * 2, height + line_width * 2);
	float psf = Desktop::GetPixelScalingFactor();
	rect.origin *= psf;
	rect.size *= psf;
	graphics_present(context.color_surface, rect.origin.x, rect.origin.y, rect.origin.x, rect.origin.y,
					 rect.size.width, rect.size.height);
	
	drawing = false;
}

void Boot::Load(NSThread* main) {
	// Create graphics context
	float psf = Desktop::GetPixelScalingFactor();
	NSApplication::SetPixelScalingFactor(psf);
	context = graphics_context_create(graphics_info.resolution_x * psf, graphics_info.resolution_y * psf, 24, 16, 0);
	
	// Setup graphics state
	NSMatrix perspective = NSMatrix::Ortho2D(0, graphics_info.resolution_x, graphics_info.resolution_y, 0);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_PROJECTION, perspective);
	NSMatrix identity = NSMatrix::Identity();
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_WORLD, identity);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, identity);
	
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDENABLE, true);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_SRCBLEND, GRAPHICS_BLENDOP_SRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_DSTBLEND, GRAPHICS_BLENDOP_INVSRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDEQUATION, GRAPHICS_BLENDEQ_ADD);
	//graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_MULTISAMPLEANTIALIAS, true);
	//graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_MULTISAMPLEMASK, 16);
	
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MINFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MAGFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	
	// Upload graphics data
	square_vbo = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(square_vbo, square, sizeof(square));
	color_vbo = graphics_buffer_create(sizeof(color), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(color_vbo, color, sizeof(color));
	white_vbo = graphics_buffer_create(sizeof(white), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(white_vbo, white, sizeof(white));
	memset(grayvao, 0, sizeof(grayvao));
	grayvao[0].bid = square_vbo;
	grayvao[0].offset = 0;
	grayvao[0].stride = 2 * sizeof(float);
	grayvao[0].type = GRAPHICS_TYPE_FLOAT2;
	grayvao[0].usage = GRAPHICS_USAGE_POSITION;
	grayvao[1].bid = color_vbo;
	grayvao[1].offset = 0;
	grayvao[1].stride = 4 * sizeof(float);
	grayvao[1].type = GRAPHICS_TYPE_FLOAT4;
	grayvao[1].usage = GRAPHICS_USAGE_COLOR;
	memset(vao, 0, sizeof(vao));
	vao[0].bid = square_vbo;
	vao[0].offset = 0;
	vao[0].stride = 2 * sizeof(float);
	vao[0].type = GRAPHICS_TYPE_FLOAT2;
	vao[0].usage = GRAPHICS_USAGE_POSITION;
	vao[1].bid = white_vbo;
	vao[1].offset = 0;
	vao[1].stride = 4 * sizeof(float);
	vao[1].type = GRAPHICS_TYPE_FLOAT4;
	vao[1].usage = GRAPHICS_USAGE_COLOR;
	
	Draw(NULL);
		
	// Start the drawing timer
	timer = NSTimer::Create(Draw, 1.0 / 60, true);
	
	// Start the loading thread
	pthread_create(&loading_thread, NULL, ThreadFunction, NULL);
}
