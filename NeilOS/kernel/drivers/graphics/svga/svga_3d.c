//
//  svga_3d.c
//  product
//
//  Created by Neil Singh on 2/2/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "svga_3d.h"
#include "svga.h"
#include <common/lib.h>
#include <common/concurrency/semaphore.h>

extern void* svga_fifo_reserve(uint32_t bytes);
extern void svga_fifo_commit_all();

// Last used number for contexts and surfaces
uint32_t context_pos = 0;
uint32_t surface_pos = 1;

mutex_t availability_lock = MUTEX_UNLOCKED;

uint32_t svga3d_find_next_context() {
	down(&availability_lock);
	uint32_t ret = context_pos;
	context_pos++;
	if (context_pos == SVGA3D_MAX_CONTEXT_IDS)
		context_pos = 0;
	up(&availability_lock);
	return ret;
}

uint32_t svga3d_find_next_surface() {
	down(&availability_lock);
	uint32_t ret = surface_pos;
	surface_pos++;
	if (surface_pos == SVGA3D_MAX_SURFACE_IDS)
		surface_pos = 1;
	up(&availability_lock);
	return ret;
}

// Helper for fifo
void* svga3d_fifo_reserve(uint32_t cmd, uint32_t cmd_size) {
	SVGA3dCmdHeader* header = svga_fifo_reserve(sizeof(SVGA3dCmdHeader) + cmd_size);
	header->id = cmd;
	header->size = cmd_size;
	return &header[1];
}

// Present rectangles on the surface
void svga3d_present(uint32_t sid, SVGA3dCopyRect* rects, uint32_t num_rects) {
	SVGA3dCmdPresent* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_PRESENT,
												sizeof(SVGA3dCmdPresent) + num_rects * sizeof(SVGA3dCopyRect));
	cmd->sid = sid;
	memcpy(&cmd[1], rects, sizeof(SVGA3dCopyRect) * num_rects);
	svga_fifo_commit_all();
}

// Present rectangles to the screen (gauaranteed to update the 2D buffer)
void svga3d_present_readback(SVGA3dCopyRect* rects, uint32_t num_rects) {
	void* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_PRESENT_READBACK, num_rects * sizeof(SVGA3dCopyRect));
	memcpy(cmd, rects, sizeof(SVGA3dCopyRect) * num_rects);
	svga_fifo_commit_all();
}

// Create a surface with the specified options and returns the surface id (sid)
// (need SVGA3D_MAX_SURFACE_FACES faces but most of the time only one will be used)
uint32_t svga3d_surface_create(svga3d_surface_info* info) {
	SVGA3dCmdDefineSurface_v2* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SURFACE_DEFINE_V2,
														 sizeof(SVGA3dCmdDefineSurface_v2) +
														 info->num_mips * sizeof(SVGA3dSize));
	uint32_t sid = svga3d_find_next_surface();
	cmd->sid = sid;
	cmd->format = info->format;
	cmd->surfaceFlags = info->flags;
	memcpy(cmd->face, info->faces, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
	cmd->multisampleCount = info->num_samples;
	cmd->autogenFilter = info->filter;
	memcpy(&cmd[1], info->mip_sizes, sizeof(SVGA3dSize) * info->num_mips);
	svga_fifo_commit_all();
	
	return sid;
}

// Copy from ram to surface's vram or vice versa
// Note: pages must stay mapped while DMA is in progress
void svga3d_surface_dma(SVGAGuestImage* guest_image, SVGA3dSurfaceImageId* host_image, SVGA3dTransferType transfer,
						SVGA3dCopyBox* boxes, uint32_t num_boxes) {
	SVGA3dCmdSurfaceDMA* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SURFACE_DMA,
													  sizeof(SVGA3dCmdSurfaceDMA) + num_boxes * sizeof(SVGA3dCopyBox));
	cmd->guest = *guest_image;
	cmd->host = *host_image;
	cmd->transfer = transfer;
	memcpy(&cmd[1], boxes, sizeof(SVGA3dCopyBox) * num_boxes);
	svga_fifo_commit_all();
}

// Copy rectangles from one surface to another
void svga3d_surface_copy(SVGA3dSurfaceImageId* src, SVGA3dSurfaceImageId* dest, SVGA3dCopyBox* boxes, uint32_t num_boxes) {
	SVGA3dCmdSurfaceCopy* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SURFACE_COPY,
												   sizeof(SVGA3dCmdSurfaceCopy) + num_boxes * sizeof(SVGA3dCopyBox));
	cmd->src = *src;
	cmd->dest = *dest;
	memcpy(&cmd[1], boxes, sizeof(SVGA3dCopyBox) * num_boxes);
	svga_fifo_commit_all();
}

// Copy a rectangle from one surface to another (stretch the image if necessary)
void svga3d_surface_stretch_copy(SVGA3dSurfaceImageId* src, SVGA3dSurfaceImageId* dest,
								 SVGA3dBox* src_box, SVGA3dBox* dst_box, SVGA3dStretchBltMode mode) {
	SVGA3dCmdSurfaceStretchBlt* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SURFACE_STRETCHBLT,
														  sizeof(SVGA3dCmdSurfaceStretchBlt));
	cmd->src = *src;
	cmd->dest = *dest;
	cmd->boxSrc = *src_box;
	cmd->boxDest = *dst_box;
	cmd->mode = mode;
	svga_fifo_commit_all();
}

// Reformat a surface
void svga3d_surface_reformat(uint32_t sid, svga3d_surface_info* info) {
	svga3d_surface_destroy(sid);
	
	SVGA3dCmdDefineSurface_v2* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SURFACE_DEFINE_V2,
														 sizeof(SVGA3dCmdDefineSurface_v2) +
														 info->num_mips * sizeof(SVGA3dSize));
	cmd->sid = sid;
	cmd->format = info->format;
	cmd->surfaceFlags = info->flags;
	memcpy(cmd->face, info->faces, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
	cmd->multisampleCount = info->num_samples;
	cmd->autogenFilter = info->filter;
	memcpy(&cmd[1], info->mip_sizes, sizeof(SVGA3dSize) * info->num_mips);
	svga_fifo_commit_all();
}

// Destroy a surface
void svga3d_surface_destroy(uint32_t sid) {
	SVGA3dCmdDestroySurface* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SURFACE_DESTROY,
													sizeof(SVGA3dCmdDestroySurface));
	cmd->sid = sid;
	svga_fifo_commit_all();
}

// Contexts
uint32_t svga3d_context_create() {
	SVGA3dCmdDefineContext* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_CONTEXT_DEFINE,
													  sizeof(SVGA3dCmdDefineContext));
	uint32_t cid = svga3d_find_next_context();
	cmd->cid = cid;
	svga_fifo_commit_all();
	
	return cid;
}

void svga3d_context_destroy(uint32_t cid) {
	SVGA3dCmdDestroyContext* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_CONTEXT_DESTROY,
													  sizeof(SVGA3dCmdDestroyContext));
	cmd->cid = cid;
	svga_fifo_commit_all();
}

// Clear rects in a context
void svga3d_clear(uint32_t cid, SVGA3dClearFlag flags, SVGA3dClearValue* value,
				  SVGA3dRect* rects, uint32_t num_rects) {
	SVGA3dCmdClear* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_CLEAR,
													   sizeof(SVGA3dCmdClear) + sizeof(SVGA3dRect) * num_rects);
	cmd->cid = cid;
	cmd->clearFlag = flags;
	cmd->color = value->color;
	cmd->depth = value->depth;
	cmd->stencil = value->stencil;
	memcpy(&cmd[1], rects, sizeof(SVGA3dRect) * num_rects);
	svga_fifo_commit_all();
}

// Draw primitives
void svga3d_draw(uint32_t cid, SVGA3dVertexDecl* decls, uint32_t num_decls,
				 SVGA3dPrimitiveRange* ranges, uint32_t num_ranges) {
	SVGA3dCmdDrawPrimitives* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_DRAW_PRIMITIVES, sizeof(SVGA3dCmdDrawPrimitives) +
													   sizeof(SVGA3dVertexDecl) * num_decls +
													   sizeof(SVGA3dPrimitiveRange) * num_ranges);
	cmd->cid = cid;
	cmd->numVertexDecls = num_decls;
	cmd->numRanges = num_ranges;
	
	SVGA3dVertexDecl* darray = (SVGA3dVertexDecl*)&cmd[1];
	SVGA3dPrimitiveRange* rarray = (SVGA3dPrimitiveRange*)&darray[num_decls];
	memcpy(darray, decls, sizeof(SVGA3dVertexDecl) * num_decls);
	memcpy(rarray, ranges, sizeof(SVGA3dPrimitiveRange) * num_ranges);
	svga_fifo_commit_all();
}

// Set the render target
void svga3d_state_render_target(uint32_t cid, SVGA3dRenderTargetType type, SVGA3dSurfaceImageId* target) {
	SVGA3dCmdSetRenderTarget* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETRENDERTARGET,
											  sizeof(SVGA3dCmdSetRenderTarget));
	cmd->cid = cid;
	cmd->type = type;
	cmd->target = *target;
	svga_fifo_commit_all();
}

// Set Z range
void svga3d_state_z_range(uint32_t cid, float z_min, float z_max) {
	SVGA3dCmdSetZRange* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETZRANGE,
														sizeof(SVGA3dCmdSetZRange));
	cmd->cid = cid;
	cmd->zRange.min = z_min;
	cmd->zRange.max = z_max;
	svga_fifo_commit_all();
}

// Set viewport
void svga3d_state_viewport(uint32_t cid, SVGA3dRect* rect) {
	SVGA3dCmdSetViewport* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETVIEWPORT,
														sizeof(SVGA3dCmdSetViewport));
	cmd->cid = cid;
	cmd->rect = *rect;
	svga_fifo_commit_all();
}

// Set scissor rect
void svga3d_state_scissor_rect(uint32_t cid, SVGA3dRect* rect) {
	SVGA3dCmdSetScissorRect* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETSCISSORRECT,
													sizeof(SVGA3dCmdSetScissorRect));
	cmd->cid = cid;
	cmd->rect = *rect;
	svga_fifo_commit_all();
}

// Set clip plane
void svga3d_state_clip_plane(uint32_t cid, uint32_t index, const float plane[4]) {
	SVGA3dCmdSetClipPlane* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETCLIPPLANE,
													   sizeof(SVGA3dCmdSetClipPlane));
	cmd->cid = cid;
	cmd->index = index;
	memcpy(cmd->plane, plane, sizeof(float) * 4);
	svga_fifo_commit_all();
}

// Set texture state
void svga3d_state_texture_state(uint32_t cid, SVGA3dTextureState* states, uint32_t num_states) {
	SVGA3dCmdSetTextureState* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETTEXTURESTATE,
									sizeof(SVGA3dCmdSetTextureState) + sizeof(SVGA3dTextureState) * num_states);
	cmd->cid = cid;
	memcpy(&cmd[1], states, sizeof(SVGA3dTextureState) * num_states);
	svga_fifo_commit_all();
}

// Set render state
void svga3d_state_render_state(uint32_t cid, SVGA3dRenderState* states, uint32_t num_states) {
	SVGA3dCmdSetRenderState* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETRENDERSTATE,
									sizeof(SVGA3dCmdSetRenderState) + sizeof(SVGA3dRenderState) * num_states);
	cmd->cid = cid;
	memcpy(&cmd[1], states, sizeof(SVGA3dRenderState) * num_states);
	svga_fifo_commit_all();
}

// Set transform
void svga3d_transform_set(uint32_t cid, SVGA3dTransformType type, const float* matrix) {
	SVGA3dCmdSetTransform* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETTRANSFORM,
													 sizeof(SVGA3dCmdSetTransform));
	cmd->cid = cid;
	cmd->type = type;
	memcpy(cmd->matrix, matrix, sizeof(float) * 16);
	svga_fifo_commit_all();
}

// Set material
void svga3d_light_material(uint32_t cid, SVGA3dFace face, const SVGA3dMaterial* material) {
	SVGA3dCmdSetMaterial* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETMATERIAL,
													 sizeof(SVGA3dCmdSetMaterial));
	cmd->cid = cid;
	cmd->face = face;
	memcpy(&cmd->material, material, sizeof(SVGA3dMaterial));
	svga_fifo_commit_all();
}

// Set light data
void svga3d_light_data(uint32_t cid, uint32_t index, const SVGA3dLightData* data) {
	SVGA3dCmdSetLightData* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETLIGHTDATA,
													sizeof(SVGA3dCmdSetLightData));
	cmd->cid = cid;
	cmd->index = index;
	memcpy(&cmd->data, data, sizeof(SVGA3dLightData));
	svga_fifo_commit_all();
}

// Set light enabled
void svga3d_light_enabled(uint32_t cid, uint32_t index, uint32_t enabled) {
	SVGA3dCmdSetLightEnabled* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SETLIGHTENABLED,
													sizeof(SVGA3dCmdSetLightEnabled));
	cmd->cid = cid;
	cmd->index = index;
	cmd->enabled = enabled;
	svga_fifo_commit_all();
}

// Create shader (returns 0 on success)
uint32_t svga3d_shader_create(uint32_t cid, uint32_t shid, SVGA3dShaderType type, const void* bytecode, uint32_t num_bytes) {
	if (num_bytes & 3)
		return -1;
	
	SVGA3dCmdDefineShader* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SHADER_DEFINE,
													sizeof(SVGA3dCmdDefineShader) + num_bytes);
	cmd->cid = cid;
	cmd->shid = shid;
	cmd->type = type;
	memcpy(&cmd[1], bytecode, num_bytes);
	svga_fifo_commit_all();
	
	return 0;
}

// Sets a uniform value in a shader
void svga3d_shader_const(uint32_t cid, uint32_t reg, SVGA3dShaderType type,
						 SVGA3dShaderConstType ctype, const void* value, uint32_t num_bytes) {
	SVGA3dCmdSetShaderConst* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SET_SHADER_CONST,
													 sizeof(SVGA3dCmdSetShaderConst));
	cmd->cid = cid;
	cmd->reg = reg;
	cmd->type = type;
	cmd->ctype = ctype;
	memset(cmd->values, 0, sizeof(float) * 4);
	memcpy(cmd->values, value, num_bytes);
	svga_fifo_commit_all();
}

// Sets the active shader for a given type
void svga3d_shader_set_active(uint32_t cid, uint32_t shid, SVGA3dShaderType type) {
	SVGA3dCmdSetShader* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SET_SHADER,
													   sizeof(SVGA3dCmdSetShader));
	cmd->cid = cid;
	cmd->shid = shid;
	cmd->type = type;
	svga_fifo_commit_all();
}

// Destroy a shader
void svga3d_shader_destroy(uint32_t cid, uint32_t shid, SVGA3dShaderType type) {
	SVGA3dCmdDestroyShader* cmd = svga3d_fifo_reserve(SVGA_3D_CMD_SHADER_DESTROY,
													 sizeof(SVGA3dCmdDestroyShader));
	cmd->cid = cid;
	cmd->shid = shid;
	cmd->type = type;
	svga_fifo_commit_all();
}
