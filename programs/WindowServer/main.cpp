//
//  main.cpp
//  WindowServer
//
//  Created by Neil Singh on 1/25/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include <stdio.h>
#include <graphics/graphics.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>
#include <NeilOS/NeilOS.h>

void* mouse_func(void* unused) {
	int fd = open("/dev/mouse", O_RDONLY);
	for (;;) {
		uint8_t buffer[9];
		read(fd, buffer, 9);
	}
}

float MDTweenEaseOutCubic(float time)
{
	float f = time - 1;
	return f * f * f + 1;
}

int main(int argc, char* argv[]) {
	int res_x = 1280;
	int res_y = 800;
	graphics_info_t info;
	info.resolution_x = res_x;
	info.resolution_y = res_y;
	graphics_info_set(&info);
	
	NSSound* sound = new NSSound("/system/sounds/boot.wav");
	sound->Play();
	
	// Set the cursor
	NSImage* image = new NSImage("/system/images/cursors/default.png");
	graphics_cursor_image_set(0, 0, int(image->GetSize().width + 0.5f), int(image->GetSize().height + 0.5f),
							  const_cast<void*>(reinterpret_cast<const void*>(image->GetPixelData())));
	delete image;
	
	pthread_t thread;
	pthread_create(&thread, nullptr, mouse_func, NULL);
	
	graphics_context_t context = graphics_context_create(res_x, res_y, 32, 16, 0);
	
	float square[] = {
		0, 0,
		(float)res_x, 0,
		(float)res_x, (float)res_y,
		
		0, 0,
		0, (float)res_y,
		(float)res_x, (float)res_y,
	};
	float square_colors[] = {
		1.0, 1.0, 1.0, 1.0,
		1.0, 1.0, 1.0, 1.0,
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
		0, 0, 0, 1.0,
		0, 0, 0, 1.0,
	};
	float uv[] = {
		0, 0,
		1.0, 0,
		1.0, 1.0,
		
		0, 0,
		0, 1,
		1.0, 1.0,
	};
	uint32_t vertices = graphics_buffer_create(sizeof(square), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(vertices, square, sizeof(square));
	uint32_t colors = graphics_buffer_create(sizeof(square_colors), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(colors, square_colors, sizeof(square_colors));
	uint32_t fade_buffer = graphics_buffer_create(sizeof(fade_colors), GRAPHICS_BUFFER_DYNAMIC);
	graphics_buffer_data(fade_buffer, fade_colors, sizeof(fade_colors));
	uint32_t uvs = graphics_buffer_create(sizeof(uv), GRAPHICS_BUFFER_STATIC);
	graphics_buffer_data(uvs, uv, sizeof(uv));
	
	image = new NSImage("/system/images/background.jpg");
	int img_width = int(image->GetSize().width + 0.5f);
	int img_height = int(image->GetSize().height + 0.5f);
	
	uint32_t img = graphics_buffer_create(img_width, img_height, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(img, image->GetPixelData(), img_width, img_height, img_width * img_height * 4);
	
	graphics_vertex_array_t vao[3];
	memset(vao, 0, sizeof(graphics_vertex_array_t) * 3);
	vao[0].bid = vertices;
	vao[0].offset = 0;
	vao[0].stride = 2 * sizeof(float);
	vao[0].type = GRAPHICS_TYPE_FLOAT2;
	vao[0].usage = GRAPHICS_USAGE_POSITION;
	vao[1].bid = colors;
	vao[1].offset = 0;
	vao[1].stride = 4 * sizeof(float);
	vao[1].type = GRAPHICS_TYPE_FLOAT4;
	vao[1].usage = GRAPHICS_USAGE_COLOR;
	vao[2].bid = uvs;
	vao[2].offset = 0;
	vao[2].stride = 2 * sizeof(float);
	vao[2].type = GRAPHICS_TYPE_FLOAT2;
	vao[2].usage = GRAPHICS_USAGE_TEXCOORD;
	
	graphics_vertex_array_t fade_vao[2];
	memset(fade_vao, 0, sizeof(graphics_vertex_array_t) * 2);
	fade_vao[0].bid = vertices;
	fade_vao[0].offset = 0;
	fade_vao[0].stride = 2 * sizeof(float);
	fade_vao[0].type = GRAPHICS_TYPE_FLOAT2;
	fade_vao[0].usage = GRAPHICS_USAGE_POSITION;
	fade_vao[1].bid = fade_buffer;
	fade_vao[1].offset = 0;
	fade_vao[1].stride = 4 * sizeof(float);
	fade_vao[1].type = GRAPHICS_TYPE_FLOAT4;
	fade_vao[1].usage = GRAPHICS_USAGE_COLOR;
	
	NSMatrix perspective = NSMatrix::Ortho2D(0, res_x, res_y, 0);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_PROJECTION, perspective);
	NSMatrix identity = NSMatrix::Identity();
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_WORLD, identity);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, identity);
	
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDENABLE, true);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_SRCBLEND, GRAPHICS_BLENDOP_SRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_DSTBLEND, GRAPHICS_BLENDOP_INVSRCALPHA);
	graphics_renderstate_seti(&context, GRAPHICS_RENDERSTATE_BLENDEQUATION, GRAPHICS_BLENDEQ_ADD);
	
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MINFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_MAGFILTER, GRAPHICS_TEXTURE_FILTER_LINEAR);
	
	float fade = 1.0;
	struct timeval start, initial_start;
	gettimeofday(&initial_start, NULL);
	unsigned int frames = 0;
	uint32_t last_fence = 0;
	while (fade > 0.0f) {
		gettimeofday(&start, NULL);
		graphics_clear(&context, 0x0, 1.0f, 0, GRAPHICS_CLEAR_COLOR | GRAPHICS_CLEAR_DEPTH,
					   0, 0, res_x, res_y);
		
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, img);
		graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLELIST, 2, vao, 3);
		graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, -1);
		graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLELIST, 2, fade_vao, 2);
		
		graphics_fence_sync(last_fence);
		graphics_present(context.color_surface, 0, 0, 0, 0, res_x, res_y);
		last_fence = graphics_fence_create();
		
		struct timeval end;
		gettimeofday(&end, NULL);
		int32_t us_left = start.tv_sec * 1000 * 1000 + start.tv_usec + (1000 * 1000 / 60) -
							(end.tv_sec * 1000 * 1000 + end.tv_usec);
		
		// Update fade color
		fade = ((initial_start.tv_sec * 1000 * 1000 + initial_start.tv_usec + (1000 * 1000) -
				 (end.tv_sec * 1000 * 1000 + end.tv_usec)) / (1000.0f * 1000.0f));
		for (int z = 0; z < 6; z++)
			fade_colors[z * 4 + 3] = fade;
		graphics_buffer_data(fade_buffer, fade_colors, sizeof(fade_colors));
		
		//if (us_left > 0)
		//	usleep(us_left);
		frames++;
	}
	
	// Temp but display total frames
	NSFont f = NSFont(50);
	NSImage* fps = f.GetImage(std::to_string(frames), NSColor<uint8_t>::RedColor());
	img_width = int(fps->GetSize().width + 0.5f);
	img_height = int(fps->GetSize().height + 0.5f);
	uint32_t fps_img = graphics_buffer_create(img_width, img_height, GRAPHICS_BUFFER_STATIC, GRAPHICS_FORMAT_A8R8G8B8);
	graphics_buffer_data(fps_img, fps->GetPixelData(), img_width, img_height, img_width * img_height * 4);
	delete fps;
	
	graphics_clear(&context, 0x0, 1.0f, 0, GRAPHICS_CLEAR_COLOR | GRAPHICS_CLEAR_DEPTH,
				   0, 0, res_x, res_y);
	
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, img);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLELIST, 2, vao, 3);
	graphics_texturestate_seti(&context, 0, GRAPHICS_TEXTURESTATE_BIND_TEXTURE, fps_img);
	NSMatrix m = NSMatrix::Identity();
	m.Scale(0.1, 0.1, 1);
	graphics_transform_set(&context, GRAPHICS_TRANSFORM_VIEW, m);
	graphics_draw(&context, GRAPHICS_PRIMITIVE_TRIANGLELIST, 2, vao, 3);
	m = NSMatrix::Identity();
	
	graphics_present(context.color_surface, 0, 0, 0, 0, res_x, res_y);
	
	for (;;) {}
	
	return 0;
}
