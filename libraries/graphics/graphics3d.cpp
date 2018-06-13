//
//  graphics3d.cpp
//  product
//
//  Created by Neil Singh on 2/5/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "graphics.h"

#include "svga.cpp"

#include <stdint.h>
#include <string.h>

// Present rectangles on the surface
extern "C" void sys_graphics_present(uint32_t sid, SVGA3dCopyRect* rects, uint32_t num_rects);
// Present rectangles to the screen (gauaranteed to update the 2D buffer)
extern "C" void sys_graphics_present_readback(SVGA3dCopyRect* rects, uint32_t num_rects);
// Create a surface with the specified options and returns the surface id (sid)
// (need SVGA3D_MAX_SURFACE_FACES faces but most of the time only one will be used)
extern "C" uint32_t sys_graphics_surface_create(uint32_t flags, uint32_t format,
												SVGA3dSurfaceFace* faces, SVGA3dSize* mipSizes, uint32_t num_mips);
// Copy from ram to surface's vram or vice versa
// Note: pages must stay mapped while DMA is in progress
extern "C" void sys_graphics_surface_dma(void* buffer, uint32_t size, SVGA3dSurfaceImageId* host_image,
									   uint32_t transfer, SVGA3dCopyBox* boxes, uint32_t num_boxes);
// Copy rectangles from one surface to another
extern "C" void sys_graphics_surface_copy(SVGA3dSurfaceImageId* src, SVGA3dSurfaceImageId* dest,
										  SVGA3dCopyBox* boxes, uint32_t num_boxes);
// Copy a rectangle from one surface to another (stretch the image if necessary)
extern "C" void sys_graphics_surface_stretch_copy(SVGA3dSurfaceImageId* src, SVGA3dSurfaceImageId* dest,
												  SVGA3dBox* src_box, SVGA3dBox* dst_box, uint32_t mode);
// Reformat a surface
extern "C" uint32_t sys_graphics_surface_reformat(uint32_t sid, uint32_t flags, uint32_t format,
												SVGA3dSurfaceFace* faces, SVGA3dSize* mipSizes, uint32_t num_mips);
// Destroy a surface
extern "C" void sys_graphics_surface_destroy(uint32_t sid);
extern "C" uint32_t sys_graphics_context_create();
extern "C" void sys_graphics_context_destroy(uint32_t cid);
// Clear rects in a context
extern "C" void sys_graphics_clear(uint32_t cid, uint32_t flags, SVGA3dClearValue* value,
								   SVGA3dRect* rects, uint32_t num_rects);
// Draw primitives
extern "C" void sys_graphics_draw(uint32_t cid, SVGA3dVertexDecl* decls, uint32_t num_decls,
								  SVGA3dPrimitiveRange* ranges, uint32_t num_ranges);
// Set the render target
extern "C" void sys_graphics_state_render_target(uint32_t cid, uint32_t type, SVGA3dSurfaceImageId* target);
// Set Z range
extern "C" void sys_graphics_state_z_range(uint32_t cid, float z_min, float z_max);
// Set viewport
extern "C" void sys_graphics_state_viewport(uint32_t cid, SVGA3dRect* rect);
// Set scissor rect
extern "C" void sys_graphics_state_scissor_rect(uint32_t cid, SVGA3dRect* rect);
// Set clip plane
extern "C" void sys_graphics_state_clip_plane(uint32_t cid, uint32_t index, const float plane[4]);
// Set texture state
extern "C" void sys_graphics_state_texture_state(uint32_t cid, SVGA3dTextureState* states, uint32_t num_states);
// Set render state
extern "C" void sys_graphics_state_render_state(uint32_t cid, SVGA3dRenderState* states, uint32_t num_states);
// Set transform
extern "C" void sys_graphics_transform_set(uint32_t cid, uint32_t type, const float* matrix);
// Set material
extern "C" void sys_graphics_light_material(uint32_t cid, uint32_t face, const SVGA3dMaterial* material);
// Set light data
extern "C" void sys_graphics_light_data(uint32_t cid, uint32_t index, const SVGA3dLightData* data);
// Set light enabled
extern "C" void sys_graphics_light_enabled(uint32_t cid, uint32_t index, uint32_t enabled);
// Create shader (returns 0 on success)
extern "C" uint32_t sys_graphics_shader_create(uint32_t cid, uint32_t shid, uint32_t type,
											   const void* bytecode, uint32_t num_bytes);
// Sets a uniform value in a active shader
extern "C" void sys_graphics_shader_const(uint32_t cid, uint32_t reg, uint32_t type,
										  uint32_t ctype, const void* value, uint32_t num_bytes);
// Sets the active shader for a given type
extern "C" void sys_graphics_shader_set_active(uint32_t cid, uint32_t shid, uint32_t type);
// Destroy a shader
extern "C" void sys_graphics_shader_destroy(uint32_t cid, uint32_t shid, uint32_t type);

// Creates a new context with the specified options (returns the context id or 0 if error)
graphics_context_t graphics_context_create(uint32_t width, uint32_t height, uint32_t color_bits, uint32_t depth_bits,
										   uint32_t stencil_bits) {
	graphics_context_t context;
	memset(&context, 0, sizeof(context));
	
	context.cid = sys_graphics_context_create();
	if (context.cid == 0)
		return context;
	
	context.width = width;
	context.height = height;
	
	// Create the surfaces
	if (color_bits) {
		SVGA3dSurfaceFormat format = SVGA3D_FORMAT_INVALID;
		if (color_bits == 24)
			format = SVGA3D_X8R8G8B8;
		else if (color_bits == 32)
			format = SVGA3D_A8R8G8B8;
		else {
			graphics_context_destroy(&context);
			return context;
		}
		
		SVGA3dSurfaceFace faces[SVGA3D_MAX_SURFACE_FACES];
		memset(faces, 0, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
		faces[0].numMipLevels = 1;
		SVGA3dSize size = { .width = width, .height = height, .depth = 1 };
		context.color_surface = sys_graphics_surface_create(SVGA3D_SURFACE_HINT_DYNAMIC | SVGA3D_SURFACE_HINT_RENDERTARGET,
															format, faces, &size, 1);
		if (!context.color_surface) {
			graphics_context_destroy(&context);
			return context;
		}
		context.color_bits = color_bits;
	}
	if (depth_bits) {
		SVGA3dSurfaceFormat format = SVGA3D_FORMAT_INVALID;
		if (stencil_bits) {
			if (depth_bits == 24 && stencil_bits == 8)
				format = SVGA3D_Z_D24S8;
			else if (depth_bits == 15 && stencil_bits == 1)
				format = SVGA3D_Z_D15S1;
			else {
				graphics_context_destroy(&context);
				return context;
			}
		} else {
			if (depth_bits == 24)
				format = SVGA3D_Z_D24X8;
			else if (depth_bits == 32)
				format = SVGA3D_Z_D32;
			else if (depth_bits == 16)
				format = SVGA3D_Z_D16;
			else {
				graphics_context_destroy(&context);
				return context;
			}
		}
		
		SVGA3dSurfaceFace faces[SVGA3D_MAX_SURFACE_FACES];
		memset(faces, 0, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
		faces[0].numMipLevels = 1;
		SVGA3dSize size = { .width = width, .height = height, .depth = 1 };
		uint32_t flags = SVGA3D_SURFACE_HINT_DYNAMIC | SVGA3D_SURFACE_HINT_RENDERTARGET;
		if (stencil_bits != 0)
			flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
		context.depth_surface = sys_graphics_surface_create((SVGA3dSurfaceFlags)flags, format, faces, &size, 1);
		if (!context.depth_surface) {
			graphics_context_destroy(&context);
			return context;
		}
		context.depth_bits = depth_bits;
		context.stencil_bits = stencil_bits;
	}
	graphics_fence_sync(graphics_fence_create());
	
	// Set render targets
	SVGA3dSurfaceImageId img;
	memset(&img, 0, sizeof(SVGA3dSurfaceImageId));
	img.sid = context.color_surface;
	sys_graphics_state_render_target(context.cid, SVGA3D_RT_COLOR0, &img);
	img.sid = context.depth_surface;
	sys_graphics_state_render_target(context.cid, SVGA3D_RT_DEPTH, &img);
	if (stencil_bits)
		sys_graphics_state_render_target(context.cid, SVGA3D_RT_STENCIL, &img);
	else {
		img.sid = -1;
		sys_graphics_state_render_target(context.cid, SVGA3D_RT_STENCIL, &img);
	}
	
	// Setup basic info
	SVGA3dRect rect = { .x = 0, .y = 0, .w = width, .h = height };
	sys_graphics_state_viewport(context.cid, &rect);
	sys_graphics_state_z_range(context.cid, 0.0f, 1.0f);
	
	SVGA3dRenderState rs[12];
	memset(rs, 0, sizeof(rs));
	rs[0].state = SVGA3D_RS_SHADEMODE;
	rs[0].uintValue = SVGA3D_SHADEMODE_SMOOTH;
	rs[1].state     = SVGA3D_RS_BLENDENABLE;
	rs[1].uintValue = false;
	rs[2].state     = SVGA3D_RS_ZENABLE;
	rs[2].uintValue = false;
	rs[3].state     = SVGA3D_RS_ZWRITEENABLE;
	rs[3].uintValue = true;
	rs[4].state     = SVGA3D_RS_ZFUNC;
	rs[4].uintValue = SVGA3D_CMP_LESS;
	rs[5].state     = SVGA3D_RS_LIGHTINGENABLE;
	rs[5].uintValue = false;
	rs[6].state     = SVGA3D_RS_CULLMODE;
	rs[6].uintValue = SVGA3D_FACE_NONE;
	rs[7].state     = SVGA3D_RS_STENCILWRITEMASK;
	rs[7].uintValue = 0xFF;
	rs[8].state     = SVGA3D_RS_STENCILMASK;
	rs[8].uintValue = 0xFF;
	rs[9].state     = SVGA3D_RS_STENCILFAIL;
	rs[9].uintValue = GRAPHICS_STENCILOP_KEEP;
	rs[10].state     = SVGA3D_RS_STENCILZFAIL;
	rs[10].uintValue = GRAPHICS_STENCILOP_KEEP;
	rs[11].state     = SVGA3D_RS_STENCILPASS;
	rs[11].uintValue = GRAPHICS_STENCILOP_KEEP;
	sys_graphics_state_render_state(context.cid, rs, sizeof(rs) / sizeof(SVGA3dRenderState));
	
	SVGA3dTextureState ts[8];
	memset(ts, 0, sizeof(SVGA3dTextureState) * 8);
	ts[0].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_COLOROP;
	ts[0].value = GRAPHICS_TC_MODULATE;
	ts[1].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_COLORARG1;
	ts[1].value = GRAPHICS_TA_TEXTURE;
	ts[2].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_COLORARG2;
	ts[2].value = GRAPHICS_TA_DIFFUSE;
	ts[3].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_ALPHAOP;
	ts[3].value = GRAPHICS_TC_SELECTARG1;
	ts[4].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_ALPHAARG1;
	ts[4].value = GRAPHICS_TA_TEXTURE;
	ts[5].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_ADDRESSU;
	ts[5].value = GRAPHICS_TEXTURE_ADDRESS_CLAMP;
	ts[6].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_ADDRESSV;
	ts[6].value = GRAPHICS_TEXTURE_ADDRESS_CLAMP;
	ts[7].name = (SVGA3dTextureStateName)GRAPHICS_TEXTURESTATE_TEXTURETRANSFORMFLAGS;
	ts[7].value = GRAPHICS_TEXTURE_TRANSFORM_S | GRAPHICS_TEXTURE_TRANSFORM_T |
		GRAPHICS_TEXTURE_TRANSFORM_R | GRAPHICS_TEXTURE_TRANSFORM_Q;
	for (int stage = 0; stage < 32; stage++) {
		for (int z = 0; z < 8; z++)
			ts[z].stage = stage;
		sys_graphics_state_texture_state(context.cid, ts, 8);
	}
	
	return context;
}

// Resize a graphics context
void graphics_context_resize(graphics_context_t* context, uint32_t width, uint32_t height) {
	SVGA3dSurfaceFormat format = SVGA3D_FORMAT_INVALID;
	if (context->color_bits == 24)
		format = SVGA3D_X8R8G8B8;
	else if (context->color_bits == 32)
		format = SVGA3D_A8R8G8B8;
	SVGA3dSurfaceFace faces[SVGA3D_MAX_SURFACE_FACES];
	memset(faces, 0, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
	faces[0].numMipLevels = 1;
	SVGA3dSize size = { .width = width, .height = height, .depth = 1 };
	sys_graphics_surface_reformat(context->color_surface, SVGA3D_SURFACE_HINT_DYNAMIC | SVGA3D_SURFACE_HINT_RENDERTARGET,
														format, faces, &size, 1);
	
	if (context->depth_bits == 24)
		format = SVGA3D_Z_D24X8;
	else if (context->depth_bits == 32)
		format = SVGA3D_Z_D32;
	else if (context->depth_bits == 16)
		format = SVGA3D_Z_D16;
	uint32_t flags = SVGA3D_SURFACE_HINT_DYNAMIC | SVGA3D_SURFACE_HINT_RENDERTARGET;
	if (context->stencil_bits != 0)
		flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
	sys_graphics_surface_reformat(context->depth_surface, (SVGA3dSurfaceFlags)flags, format, faces, &size, 1);
	
	context->width = width;
	context->height = height;
	
	SVGA3dSurfaceImageId img;
	memset(&img, 0, sizeof(SVGA3dSurfaceImageId));
	img.sid = context->color_surface;
	sys_graphics_state_render_target(context->cid, SVGA3D_RT_COLOR0, &img);
	img.sid = context->depth_surface;
	sys_graphics_state_render_target(context->cid, SVGA3D_RT_DEPTH, &img);
	if (context->stencil_bits)
		sys_graphics_state_render_target(context->cid, SVGA3D_RT_STENCIL, &img);
	
	SVGA3dRect rect = { .x = 0, .y = 0, .w = width, .h = height };
	sys_graphics_state_viewport(context->cid, &rect);
}

// Destroys a context
void graphics_context_destroy(graphics_context_t* context) {
	if (context->color_surface)
		sys_graphics_surface_destroy(context->color_surface);
	if (context->depth_surface)
		sys_graphics_surface_destroy(context->depth_surface);
	if (context->cid)
		sys_graphics_context_destroy(context->cid);
	memset(context, 0, sizeof(graphics_context_t));
}

// Creates a new buffer (in VRAM) with the specified options (returns buffer id or 0 if error).
uint32_t graphics_buffer_create(uint32_t size, uint32_t flags) {
	return graphics_buffer_create(size, 1, flags, SVGA3D_BUFFER);
}

// Creates a new buffer (in VRAM) witht the specified options and format
uint32_t graphics_buffer_create(uint32_t size, uint32_t flags, uint32_t format) {
	return graphics_buffer_create(size, 1, flags, format);
}

// Creates a new 2D buffer (in VRAM) with the specified options and format
uint32_t graphics_buffer_create(uint32_t width, uint32_t height, uint32_t flags, uint32_t format) {
	SVGA3dSurfaceFace faces[SVGA3D_MAX_SURFACE_FACES];
	memset(faces, 0, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
	faces[0].numMipLevels = 1;
	SVGA3dSize mip_sizes;
	mip_sizes.width = width;
	mip_sizes.height = height;
	mip_sizes.depth = 1;
	uint32_t sid = sys_graphics_surface_create(flags, format, faces, &mip_sizes, 1);
	graphics_fence_sync(graphics_fence_create());
	return sid;
}

// Upload data to a buffer
void graphics_buffer_data(uint32_t bid, const void* data, uint32_t size) {
	graphics_buffer_sub_data(bid, data, 0, size);
}

// Read data from a buffer
void graphics_read_data(uint32_t bid, void* data, uint32_t size) {
	graphics_read_data(bid, data, size, 1, size);
}

// Read 2d data form a buffer
void graphics_read_data(uint32_t bid, void* data, uint32_t width, uint32_t height, uint32_t size) {
	SVGA3dSurfaceImageId img;
	memset(&img, 0, sizeof(SVGA3dSurfaceImageId));
	img.sid = bid;
	SVGA3dCopyBox box;
	memset(&box, 0, sizeof(SVGA3dCopyBox));
	box.w = width;
	box.h = height;
	box.d = 1;
	sys_graphics_surface_dma(data, size, &img, SVGA3D_READ_HOST_VRAM, &box, 1);
}

// Upload data to a buffer at an offset
void graphics_buffer_sub_data(uint32_t bid, const void* data, uint32_t offset, uint32_t size) {
	SVGA3dSurfaceImageId img;
	memset(&img, 0, sizeof(SVGA3dSurfaceImageId));
	img.sid = bid;
	SVGA3dCopyBox box;
	memset(&box, 0, sizeof(SVGA3dCopyBox));
	box.x = offset;
	box.w = size;
	box.h = 1;
	box.d = 1;
	sys_graphics_surface_dma((void*)data, size, &img, SVGA3D_WRITE_HOST_VRAM, &box, 1);
}

// Upload 2d data to a buffer
void graphics_buffer_data(uint32_t bid, const void* data, uint32_t width, uint32_t height, uint32_t size) {
	graphics_buffer_sub_data(bid, data, 0, 0, width, height, size);
}

// Upload 2d data to a buffer at an offset
void graphics_buffer_sub_data(uint32_t bid, const void* data, uint32_t x, uint32_t y,
							  uint32_t width, uint32_t height, uint32_t size) {
	SVGA3dSurfaceImageId img;
	memset(&img, 0, sizeof(SVGA3dSurfaceImageId));
	img.sid = bid;
	SVGA3dCopyBox box;
	memset(&box, 0, sizeof(SVGA3dCopyBox));
	box.x = x;
	box.y = y;
	box.w = width;
	box.h = height;
	box.d = 1;
	sys_graphics_surface_dma((void*)data, size, &img, SVGA3D_WRITE_HOST_VRAM, &box, 1);
}

// Change size / format of buffer
// Note: may invalidate buffer contents
void graphics_buffer_reformat(uint32_t bid, uint32_t size, uint32_t flags) {
	graphics_buffer_reformat(bid, size, 1, flags, SVGA3D_BUFFER);
}

void graphics_buffer_reformat(uint32_t bid, uint32_t size, uint32_t flags, uint32_t format) {
	graphics_buffer_reformat(bid, size, 1, flags, format);
}

void graphics_buffer_reformat(uint32_t bid, uint32_t width, uint32_t height, uint32_t flags, uint32_t format) {
	SVGA3dSurfaceFace faces[SVGA3D_MAX_SURFACE_FACES];
	memset(faces, 0, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
	faces[0].numMipLevels = 1;
	SVGA3dSize mip_sizes;
	mip_sizes.width = width;
	mip_sizes.height = height;
	mip_sizes.depth = 1;
	sys_graphics_surface_reformat(bid, flags, format, faces, &mip_sizes, 1);
	graphics_fence_sync(graphics_fence_create());
}

// Copy data from one buffer to another
void graphics_buffer_copy(uint32_t dest_bid, uint32_t src_bid, uint32_t dest_x, uint32_t dest_y,
						  uint32_t src_x, uint32_t src_y, uint32_t width, uint32_t height) {
	SVGA3dSurfaceImageId dest, src;
	memset(&dest, 0, sizeof(dest));
	dest.sid = dest_bid;
	memset(&src, 0, sizeof(src));
	src.sid = src_bid;
	
	SVGA3dCopyBox box;
	box.srcx = src_x;
	box.srcy = src_y;
	box.srcz = 0;
	box.x = dest_x;
	box.y = dest_y;
	box.z = 0;
	box.w = width;
	box.h = height;
	box.d = 1;
	
	sys_graphics_surface_copy(&src, &dest, &box, 1);
}

// Destroy a buffer
void graphics_buffer_destroy(uint32_t bid) {
	sys_graphics_surface_destroy(bid);
}

// Clear section
void graphics_clear(graphics_context_t* context, uint32_t color, float depth, uint32_t stencil, uint32_t bits,
					uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	SVGA3dClearValue value = { .color = color, .depth = depth, .stencil = stencil };
	SVGA3dRect rect = { .x = x, .y = y, .w = width, .h = height };
	sys_graphics_clear(context->cid, bits, &value, &rect, 1);
}

// Draw primitives
void graphics_draw(graphics_context_t* context, uint32_t primitive_type, uint32_t primitive_count,
				   graphics_vertex_array_t* arrays, uint32_t num_arrays) {
	SVGA3dPrimitiveRange range;
	memset(&range, 0, sizeof(SVGA3dPrimitiveRange));
	range.indexArray.surfaceId = -1;
	range.primType = (SVGA3dPrimitiveType)primitive_type;
	range.primitiveCount = primitive_count;
	
	SVGA3dVertexDecl decls[num_arrays];
	memset(decls, 0, sizeof(SVGA3dVertexDecl) * num_arrays);
	for (uint32_t z = 0; z < num_arrays; z++) {
		decls[z].array.surfaceId = arrays[z].bid;
		decls[z].array.offset = arrays[z].offset;
		decls[z].array.stride = arrays[z].stride;
		
		decls[z].identity.type = (SVGA3dDeclType)arrays[z].type;
		decls[z].identity.usage = (SVGA3dDeclUsage)arrays[z].usage;
	}
	
	sys_graphics_draw(context->cid, decls, num_arrays, &range, 1);
}

// Draw indexed primitives
void graphics_draw_elements(graphics_context_t* context, uint32_t primitive_type, uint32_t primitive_count,
							graphics_vertex_array_t* indices, graphics_vertex_array_t* arrays, uint32_t num_arrays) {
	SVGA3dPrimitiveRange range;
	memset(&range, 0, sizeof(SVGA3dPrimitiveRange));
	range.indexArray.surfaceId = indices->bid;
	range.indexArray.stride = indices->stride;
	range.indexArray.offset = indices->offset;
	range.indexWidth = indices->stride;
	range.primType = (SVGA3dPrimitiveType)primitive_type;
	range.primitiveCount = primitive_count;
	
	SVGA3dVertexDecl decls[num_arrays];
	memset(decls, 0, sizeof(SVGA3dVertexDecl) * num_arrays);
	for (uint32_t z = 0; z < num_arrays; z++) {
		decls[z].array.surfaceId = arrays[z].bid;
		decls[z].array.offset = arrays[z].offset;
		decls[z].array.stride = arrays[z].stride;
		
		decls[z].identity.type = (SVGA3dDeclType)arrays[z].type;
		decls[z].identity.usage = (SVGA3dDeclUsage)arrays[z].usage;
	}
	
	sys_graphics_draw(context->cid, decls, num_arrays, &range, 1);
}

// Set matrix transform
void graphics_transform_set(graphics_context_t* context, uint32_t type, const NSMatrix& matrix) {
	sys_graphics_transform_set(context->cid, type, matrix.GetData());
}

// Set render state
void graphics_renderstate_seti(graphics_context_t* context, uint32_t type, uint32_t value) {
	SVGA3dRenderState state;
	state.state = (SVGA3dRenderStateName)type;
	state.uintValue = value;
	sys_graphics_state_render_state(context->cid, &state, 1);
}

void graphics_renderstate_setf(graphics_context_t* context, uint32_t type, float value) {
	SVGA3dRenderState state;
	state.state = (SVGA3dRenderStateName)type;
	state.floatValue = value;
	sys_graphics_state_render_state(context->cid, &state, 1);
}

// Set texture state
void graphics_texturestate_seti(graphics_context_t* context, uint32_t stage, uint32_t type, uint32_t value) {
	SVGA3dTextureState state;
	state.stage = stage;
	state.name = (SVGA3dTextureStateName)type;
	state.value = value;
	sys_graphics_state_texture_state(context->cid, &state, 1);
}

void graphics_texturestate_setf(graphics_context_t* context, uint32_t stage, uint32_t type, uint32_t value) {
	SVGA3dTextureState state;
	state.stage = stage;
	state.name = (SVGA3dTextureStateName)type;
	state.floatValue = value;
	sys_graphics_state_texture_state(context->cid, &state, 1);
}

// Set render target
void graphics_rendertarget_set(graphics_context_t* context, int target, int bid) {
	SVGA3dSurfaceImageId img;
	memset(&img, 0, sizeof(SVGA3dSurfaceImageId));
	img.sid = bid;
	sys_graphics_state_render_target(context->cid, target, &img);
}

// Set viewport
void graphics_viewport_set(graphics_context_t* context, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	SVGA3dRect rect = { .x = x, .y = y, .w = width, .h = height };
	sys_graphics_state_viewport(context->cid, &rect);
}

// Set scissor rect
void graphics_scissor_rect_set(graphics_context_t* context, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	SVGA3dRect rect = { .x = x, .y = y, .w = width, .h = height };
	sys_graphics_state_scissor_rect(context->cid, &rect);
}

// Present surface to screen
void graphics_present(uint32_t sid, uint32_t screen_x, uint32_t screen_y,
					  uint32_t src_x, uint32_t src_y, uint32_t width, uint32_t height) {
	SVGA3dCopyRect rect = { .x = screen_x, .y = screen_y, .w = width, .h = height, .srcx = src_x, .srcy = src_y };
	sys_graphics_present(sid, &rect, 1);
}

// Update the 2D framebuffer with the most recent data
void graphics_present_readback(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	SVGA3dCopyRect rect = { .x = x, .y = y, .w = width, .h = height, .srcx = 0, .srcy = 0 };
	sys_graphics_present_readback(&rect, 1);
}
