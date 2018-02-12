//
//  svga_3d.h
//  product
//
//  Created by Neil Singh on 2/2/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#ifndef SVGA_3D_H
#define SVGA_3D_H

#include <common/types.h>
#include "svga3d_reg.h"

// Present rectangles on the surface
void svga3d_present(uint32_t sid, SVGA3dCopyRect* rects, uint32_t num_rects);
// Present rectangles to the screen (gauaranteed to update the 2D buffer)
void svga3d_present_readback(SVGA3dCopyRect* rects, uint32_t num_rects);

// Surfaces

// Create a surface with the specified options and returns the surface id (sid)
// (need SVGA3D_MAX_SURFACE_FACES faces but most of the time only one will be used)
uint32_t svga3d_surface_create(SVGA3dSurfaceFlags flags, SVGA3dSurfaceFormat format, SVGA3dSurfaceFace* faces,
						   SVGA3dSize* mipSizes, uint32_t num_mips);
// Copy from ram to surface's vram or vice versa
// Note: pages must stay mapped while DMA is in progress
void svga3d_surface_dma(SVGA3dGuestImage* guest_image, SVGA3dSurfaceImageId* host_image, SVGA3dTransferType transfer,
						SVGA3dCopyBox* boxes, uint32_t num_boxes);
// Copy rectangles from one surface to another
void svga3d_surface_copy(SVGA3dSurfaceImageId* src, SVGA3dSurfaceImageId* dest, SVGA3dCopyBox* boxes, uint32_t num_boxes);
// Copy a rectangle from one surface to another (stretch the image if necessary)
void svga3d_surface_stretch_copy(SVGA3dSurfaceImageId* src, SVGA3dSurfaceImageId* dest,
								 SVGA3dBox* src_box, SVGA3dBox* dst_box, SVGA3dStretchBltMode mode);
// Destroy a surface
void svga3d_surface_destroy(uint32_t sid);

// Contexts
uint32_t svga3d_context_create();
void svga3d_context_destroy(uint32_t cid);

// Drawing

// Clear rects in a context
void svga3d_clear(uint32_t cid, SVGA3dClearFlag flags, SVGA3dClearValue* value,
				  SVGA3dRect* rects, uint32_t num_rects);
// Draw primitives
void svga3d_draw(uint32_t cid, SVGA3dVertexDecl* decls, uint32_t num_decls,
				 SVGA3dPrimitiveRange* ranges, uint32_t num_ranges);

// Render states

// Set the render target
void svga3d_state_render_target(uint32_t cid, SVGA3dRenderTargetType type, SVGA3dSurfaceImageId* target);
// Set Z range
void svga3d_state_z_range(uint32_t cid, float z_min, float z_max);
// Set viewport
void svga3d_state_viewport(uint32_t cid, SVGA3dRect* rect);
// Set scissor rect
void svga3d_state_scissor_rect(uint32_t cid, SVGA3dRect* rect);
// Set clip plane
void svga3d_state_clip_plane(uint32_t cid, uint32_t index, const float plane[4]);
// Set texture state
void svga3d_state_texture_state(uint32_t cid, SVGA3dTextureState* states, uint32_t num_states);
// Set render state
void svga3d_state_render_state(uint32_t cid, SVGA3dRenderState* states, uint32_t num_states);

// Fixed Function

// Set transform
void svga3d_transform_set(uint32_t cid, SVGA3dTransformType type, const float* matrix);
// Set material
void svga3d_light_material(uint32_t cid, SVGA3dFace face, const SVGA3dMaterial* material);
// Set light data
void svga3d_light_data(uint32_t cid, uint32_t index, const SVGA3dLightData* data);
// Set light enabled
void svga3d_light_enabled(uint32_t cid, uint32_t index, uint32_t enabled);

// Shaders (from DX9 bytecode)

// Create shader (returns 0 on success)
uint32_t svga3d_shader_create(uint32_t cid, uint32_t shid, SVGA3dShaderType type, const void* bytecode, uint32_t num_bytes);
// Sets a uniform value in a active shader
void svga3d_shader_const(uint32_t cid, uint32_t reg, SVGA3dShaderType type,
						 SVGA3dShaderConstType ctype, const void* value, uint32_t num_bytes);
// Sets the active shader for a given type
void svga3d_shader_set_active(uint32_t cid, uint32_t shid, SVGA3dShaderType type);
// Destroy a shader
void svga3d_shader_destroy(uint32_t cid, uint32_t shid, SVGA3dShaderType type);

#endif /* SVGA_3D_H */
