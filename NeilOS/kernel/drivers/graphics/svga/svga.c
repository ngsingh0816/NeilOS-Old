//
//  svga.c
//  product
//
//  Created by Neil Singh on 1/22/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "svga.h"
#include "svga_reg.h"
#include "svga_3d.h"
#include <drivers/pci/pci.h>
#include <common/lib.h>
#include <memory/memory.h>
#include <common/concurrency/semaphore.h>
#include <program/task.h>

#define SVGA_PCI_ENABLE_BUSMASTER		0x07

#define DEFAULT_RESOLUTION_WIDTH		1280
#define DEFAULT_RESOLUTION_HEIGHT		720
// Must be 32 for QEMU
#define DEFAULT_BPP						32

// Settings
uint32_t res_width = DEFAULT_RESOLUTION_WIDTH;
uint32_t res_height = DEFAULT_RESOLUTION_HEIGHT;
uint32_t bpp = DEFAULT_BPP;

// Device Regs
uint32_t io_base = 0;
uint8_t* fb_base = NULL;
uint32_t* fifo_base = NULL;
uint32_t vram_size = 0;
uint32_t fb_size = 0;
uint32_t fifo_size = 0;
uint32_t capabilities = 0;
uint32_t pitch = 0;
bool enabled = false;

// FIFO
semaphore_t fifo_lock = MUTEX_UNLOCKED;
uint32_t fifo_reserved_size = 0;
uint32_t next_fence = 0;
uint8_t* bounce_buffer = NULL;
void* svga_fifo_reserve(uint32_t bytes);
void* svga_fifo_reserve_cmd(uint32_t type, uint32_t bytes);
void* svga_fifo_reserve_escape(uint32_t nsid, uint32_t bytes);
void svga_fifo_commit(uint32_t bytes);
void svga_fifo_commit_all();

static inline void svga_write(uint32_t reg, uint32_t data) {
	outl(reg, io_base + SVGA_INDEX_PORT);
	outl(data, io_base + SVGA_VALUE_PORT);
}

static inline uint32_t svga_read(uint32_t reg) {
	outl(reg, io_base + SVGA_INDEX_PORT);
	return inl(io_base + SVGA_VALUE_PORT);
}

/*typedef struct {
	float position[3];
	uint32_t color;
} MyVertex;

static const MyVertex vertexData[] = {
	{ {-1, -1, -1}, 0x7FFFFFFF },
	{ {-1, -1,  1}, 0x7FFFFF00 },
	{ {-1,  1, -1}, 0x7FFF00FF },
	{ {-1,  1,  1}, 0x7FFF0000 },
	{ { 1, -1, -1}, 0x7F00FFFF },
	{ { 1, -1,  1}, 0x7F00FF00 },
	{ { 1,  1, -1}, 0x7F0000FF },
	{ { 1,  1,  1}, 0x7F000000 },
};

#define QUAD(a,b,c,d) a, b, d, d, c, a

static const uint16_t indexData[] = {
	QUAD(0,1,2,3), // -X
	QUAD(4,5,6,7), // +X
	QUAD(0,1,4,5), // -Y
	QUAD(2,3,6,7), // +Y
	QUAD(0,2,4,6), // -Z
	QUAD(1,3,5,7), // +Z
};

#undef QUAD

const uint32_t numTriangles = sizeof indexData / sizeof indexData[0] / 3;

#include "../graphics.h"

void *
SVGA_AllocGMR(uint32_t size,        // IN
			  SVGAGuestPtr *ptr)  // OUT
{
	static SVGAGuestPtr nextPtr = { SVGA_GMR_FRAMEBUFFER, 0 };
	*ptr = nextPtr;
	nextPtr.offset += size;
	return fb_base + ptr->offset;
}

void
SVGA3DUtil_SurfaceDMA2D(uint32_t sid,                   // IN
						SVGAGuestPtr *guestPtr,       // IN
						SVGA3dTransferType transfer,  // IN
						uint32_t width,                 // IN
						uint32_t height)                // IN
{
	SVGA3dCopyBox boxes;
	SVGA3dGuestImage guestImage;
	SVGA3dSurfaceImageId hostImage;
	memset(&hostImage, 0, sizeof(SVGA3dSurfaceImageId));
	hostImage.sid = sid;
	
	memset(&guestImage, 0, sizeof(SVGA3dGuestImage));
	guestImage.ptr = *guestPtr;
	guestImage.pitch = 0;
	memset(&boxes, 0, sizeof(SVGA3dCopyBox));
	boxes.w = width;
	boxes.h = height;
	boxes.d = 1;
	
	svga3d_surface_dma(&guestImage, &hostImage, transfer, &boxes, 1);
}

uint32_t define_static_buffer(void* data, uint32_t size) {
	SVGA3dSurfaceFace faces[SVGA3D_MAX_SURFACE_FACES];
	memset(faces, 0, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
	faces[0].numMipLevels = 1;
	SVGA3dSize mip_sizes;
	mip_sizes.width = size;
	mip_sizes.height = 1;
	mip_sizes.depth = 1;
	uint32_t sid = svga3d_surface_create(0, SVGA3D_BUFFER, faces, &mip_sizes, 1);
//	SVGAGuestPtr ptr;
//	void* buffer = SVGA_AllocGMR(size, &ptr);
//	memcpy(buffer, data, size);
//
//	SVGA3DUtil_SurfaceDMA2D(sid, &ptr, SVGA3D_WRITE_HOST_VRAM, size, 1);
//	svga_fence_sync(svga_fence_insert());
	
	SVGA3dSurfaceImageId hostImage;
	memset(&hostImage, 0, sizeof(SVGA3dSurfaceImageId));
	hostImage.sid = sid;
	SVGA3dCopyBox boxes;
	memset(&boxes, 0, sizeof(SVGA3dCopyBox));
	boxes.w = size;
	boxes.h = 1;
	boxes.d = 1;
	graphics3d_surface_dma(data, size, &hostImage, SVGA3D_WRITE_HOST_VRAM, &boxes, 1);
	
	return sid;
}*/

// Initializes the VMWare SVGA-II device and enables it
bool svga_init() {
	pci_device_t dev = pci_device_info_vendor(PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_SVGA2);
	if (dev.bus == (uint8_t)-1)
		return false;
	pci_set_command(&dev, SVGA_PCI_ENABLE_BUSMASTER);
	io_base = pci_interpret_bar(dev.bar[0]);
	fb_base = (uint8_t*)pci_interpret_bar(dev.bar[1]);
	fifo_base = (uint32_t*)pci_interpret_bar(dev.bar[2]);
	
	// Set the max supported version
	uint32_t version = SVGA_ID_2;
	while (version >= SVGA_ID_0) {
		svga_write(SVGA_REG_ID, version);
		if (svga_read(SVGA_REG_ID) == version)
			break;
		version--;
	}
	if (version < SVGA_ID_0)
		return false;
	
	// Get some info
	vram_size = svga_read(SVGA_REG_VRAM_SIZE);
	fb_size = svga_read(SVGA_REG_FB_SIZE);
	fifo_size = svga_read(SVGA_REG_MEM_SIZE);
	capabilities = svga_read(SVGA_REG_CAPABILITIES);
	
	// Map in the virtual memory for the memory mapped I/O
	// this will not get unmapped so we don't have to worry about it (because they are kernel pages)
	// TODO: maybe map these in on demand?
	vm_lock();
	uint32_t vaddr = vm_get_next_unmapped_pages((fb_size - 1) / FOUR_MB_SIZE + 1, VIRTUAL_MEMORY_KERNEL);
	uint32_t paddr = (uint32_t)fb_base - ((uint32_t)fb_base % FOUR_MB_SIZE);
	for (uint32_t z = 0; z < fb_size; z += FOUR_MB_SIZE)
		vm_map_page(vaddr + z, paddr + z, MEMORY_RW | MEMORY_KERNEL, false);
	fb_base = (uint8_t*)((uint32_t)fb_base - paddr + vaddr);
	vaddr = vm_get_next_unmapped_pages((fifo_size - 1) / FOUR_MB_SIZE + 1, VIRTUAL_MEMORY_KERNEL);
	paddr = (uint32_t)fifo_base - ((uint32_t)fifo_base % FOUR_MB_SIZE);
	for (uint32_t z = 0; z < fifo_size; z += FOUR_MB_SIZE)
		vm_map_page(vaddr + z, paddr + z, MEMORY_RW | MEMORY_KERNEL, false);
	fifo_base = (uint32_t*)((uint32_t)fifo_base - paddr + vaddr);
	vm_unlock();
	
	svga_enable();
	
	/*svga_set_mode(res_width, res_height, bpp);
	
	// Set whole screen to black
	memset(fb_base, 0x00, res_width * res_height * 4);
	svga_update(0, 0, res_width, res_height);*/
	
	return true;
	
	/*SVGA3dSurfaceFace faces[SVGA3D_MAX_SURFACE_FACES];
	memset(faces, 0, sizeof(SVGA3dSurfaceFace) * SVGA3D_MAX_SURFACE_FACES);
	faces[0].numMipLevels = 1;
	SVGA3dSize mip_sizes;
	mip_sizes.width = 1280;
	mip_sizes.height = 720;
	mip_sizes.depth = 1;
	uint32_t csid = svga3d_surface_create(0, SVGA3D_A8R8G8B8, faces, &mip_sizes, 1);
	uint32_t dsid = svga3d_surface_create(0, SVGA3D_Z_D16, faces, &mip_sizes, 1);
	uint32_t cid = svga3d_context_create();
	SVGA3dSurfaceImageId simg;
	memset(&simg, 0, sizeof(simg));
	simg.sid = csid;
	svga3d_state_render_target(cid, SVGA3D_RT_COLOR0, &simg);
	simg.sid = dsid;
	svga3d_state_render_target(cid, SVGA3D_RT_DEPTH, &simg);
	SVGA3dRect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = 1280;
	rect.h = 720;
	svga3d_state_viewport(cid, &rect);
	svga3d_state_z_range(cid, 0.0f, 1.0f);
	SVGA3dRenderState rs[11];
	memset(&rs, 0, sizeof(SVGA3dRenderState) * 11);
	rs[0].state = SVGA3D_RS_SHADEMODE;
	rs[0].uintValue = SVGA3D_SHADEMODE_SMOOTH;
	rs[1].state     = SVGA3D_RS_BLENDENABLE;
	rs[1].uintValue = false;
	rs[2].state     = SVGA3D_RS_ZENABLE;
	rs[2].uintValue = true;
	rs[3].state     = SVGA3D_RS_ZWRITEENABLE;
	rs[3].uintValue = true;
	rs[4].state     = SVGA3D_RS_ZFUNC;
	rs[4].uintValue = SVGA3D_CMP_LESS;
	rs[5].state     = SVGA3D_RS_LIGHTINGENABLE;
	rs[5].uintValue = false;
	rs[6].state     = SVGA3D_RS_CULLMODE;
	rs[6].uintValue = SVGA3D_FACE_NONE;
	
	// Blending
//	rs[7].state     = SVGA3D_RS_BLENDENABLE;
//	rs[7].uintValue = true;
//	rs[8].state     = SVGA3D_RS_SRCBLEND;
//	rs[8].uintValue = SVGA3D_BLENDOP_SRCALPHA;
//	rs[9].state     = SVGA3D_RS_DSTBLEND;
//	rs[9].uintValue = SVGA3D_BLENDOP_INVSRCALPHA;
//	rs[10].state     = SVGA3D_RS_BLENDEQUATION;
//	rs[10].uintValue = SVGA3D_BLENDEQ_ADD;
	svga3d_state_render_state(cid, rs, 7);
	
	uint32_t vertex_sid = define_static_buffer((void*)vertexData, sizeof(vertexData));
	uint32_t index_sid = define_static_buffer((void*)indexData, sizeof(indexData));
	
	Matrix perspective;
	Matrix_Perspective(perspective, 45.0f, 1280.0f / 720.0f, 0.01f, 100.0f);
	
	Matrix view;
	
	Matrix_Copy(view, gIdentityMatrix);
	Matrix_Scale(view, 0.5, 0.5, 0.5, 1.0);
	//Matrix_RotateY(view, frame++ * 0.01f);
	Matrix_Translate(view, 0, 0, 3);
	svga3d_transform_set(cid, SVGA3D_TRANSFORM_PROJECTION, perspective);
	svga3d_transform_set(cid, SVGA3D_TRANSFORM_WORLD, gIdentityMatrix);
	svga3d_transform_set(cid, SVGA3D_TRANSFORM_VIEW, view);
	
	
	//uint32_t frame = 0;
	uint32_t fence = 0;
	uint32_t fs = 0;
	uint32_t fps = 0;
	for (;;) {
		svga_fence_sync(fence);
		
		SVGA3dClearValue value;
		value.color = 0x113366;
		value.depth = 1.0f;
		value.stencil = 0;
		svga3d_clear(cid, SVGA3D_CLEAR_COLOR | SVGA3D_CLEAR_DEPTH, &value, &rect, 1);
		
		SVGA3dVertexDecl decls[2];
		SVGA3dPrimitiveRange ranges;
		memset(decls, 0, sizeof(SVGA3dVertexDecl) * 2);
		memset(&ranges, 0, sizeof(SVGA3dPrimitiveRange));
		decls[0].identity.type = SVGA3D_DECLTYPE_FLOAT3;
		decls[0].identity.usage = SVGA3D_DECLUSAGE_POSITION;
		decls[0].array.surfaceId = vertex_sid;
		decls[0].array.stride = 16;
		decls[0].array.offset = 0;
		
		decls[1].identity.type = SVGA3D_DECLTYPE_D3DCOLOR;
		decls[1].identity.usage = SVGA3D_DECLUSAGE_COLOR;
		decls[1].array.surfaceId = vertex_sid;
		decls[1].array.stride = 16;
		decls[1].array.offset = 3 * sizeof(float);
		
		ranges.primType = SVGA3D_PRIMITIVE_TRIANGLELIST;
		ranges.primitiveCount = numTriangles;
		ranges.indexArray.surfaceId = index_sid;
		ranges.indexArray.stride = sizeof(uint16_t);
		ranges.indexWidth = sizeof(uint16_t);
		svga3d_draw(cid, decls, 2, &ranges, 1);
		
		SVGA3dCopyRect crect;
		memset(&crect, 0, sizeof(crect));
		crect.w = 1280;
		crect.h = 720;
		svga3d_present(csid, &crect, 1);
		
		fence = svga_fence_insert();
		
		for (int z = 0; z < 1000000; z++) {}
		
		fs++;
		static uint32_t val = 0;
		if (val != get_current_unix_time().val) {
			fps = fs;
			fs = 0;
			val = get_current_unix_time().val;
		}
	}
	
	return true;*/
}

// Enables the device
void svga_enable() {
	// Init FIFO
	fifo_base[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * sizeof(uint32_t);
	fifo_base[SVGA_FIFO_MAX] = fifo_size;
	fifo_base[SVGA_FIFO_NEXT_CMD] = fifo_base[SVGA_FIFO_MIN];
	fifo_base[SVGA_FIFO_STOP] = fifo_base[SVGA_FIFO_MIN];
	
	// Enable 3D
	fifo_base[SVGA_FIFO_GUEST_3D_HWVERSION] = SVGA3D_HWVERSION_CURRENT;
	
	// Enable FIFO and device
	svga_write(SVGA_REG_ENABLE, true);
	svga_write(SVGA_REG_CONFIG_DONE, true);
	enabled = true;
	
	if (fifo_base[SVGA_FIFO_3D_HWVERSION] == 0)
		printf("3D Driver Disabled.\n");
}

// Disables the device (returns to text printing)
void svga_disable() {
	svga_write(SVGA_REG_ENABLE, false);
	enabled = false;
}

bool svga_enabled() {
	return enabled;
}

// Pointer to the framebuffer
uint8_t* svga_framebuffer() {
	return &fb_base[svga_read(SVGA_REG_FB_OFFSET)];
}

uint32_t svga_framebuffer_size() {
	return fb_size;
}

// Set the resolution and bpp
void svga_set_mode(uint32_t width, uint32_t height, uint32_t bits) {
	res_width = width;
	res_height = height;
	bpp = bits;
	
	svga_write(SVGA_REG_WIDTH, width);
	svga_write(SVGA_REG_HEIGHT, height);
	svga_write(SVGA_REG_BITS_PER_PIXEL, bpp);
	svga_write(SVGA_REG_ENABLE, true);
	pitch = svga_read(SVGA_REG_BITS_PER_PIXEL);
}

uint32_t svga_resolution_get_x() {
	return res_width;
}

uint32_t svga_resolution_get_y() {
	return res_height;
}

uint32_t svga_bpp_get() {
	return bpp;
}

// Reserve space for fifo commands
void* svga_fifo_reserve(uint32_t bytes) {
	// Unlocked in fifo_commit
	down(&fifo_lock);
	uint32_t max = fifo_base[SVGA_FIFO_MAX];
	uint32_t min = fifo_base[SVGA_FIFO_MIN];
	uint32_t next_cmd = fifo_base[SVGA_FIFO_NEXT_CMD];
	
	fifo_reserved_size = bytes;
	for (;;) {
		uint32_t stop = fifo_base[SVGA_FIFO_STOP];
		bool success = false;
		if (next_cmd >= stop) {
			if ((next_cmd + bytes < max) || ((next_cmd + bytes == max) && stop > min)) {
				// There is space in the FIFO
				success = true;
			} else if (!((max - next_cmd) + (stop - min) <= bytes)) {
				// Need bounce buffer
				bounce_buffer = kmalloc(bytes);
				return bounce_buffer;
			}
		} else if (next_cmd + bytes < stop) {
			// There is space in the FIFO
			success = true;
		}
		
		if (success) {
			fifo_base[SVGA_FIFO_RESERVED] = bytes;
			return (void*)(next_cmd + (uint8_t*)fifo_base);
		} else {
			// Sync the FIFO and keep trying
			svga_write(SVGA_REG_SYNC, 1);
			svga_read(SVGA_REG_BUSY);
		}
	}
	
	return NULL;
}

// Reserve space for a fifo command of the given type
void* svga_fifo_reserve_cmd(uint32_t type, uint32_t bytes) {
	uint32_t* cmd = svga_fifo_reserve(sizeof(type) + bytes);
	cmd[0] = type;
	return (void*)(cmd + 1);
}

// Reserve space for an escape fifo command
void* svga_fifo_reserve_escape(uint32_t nsid, uint32_t bytes) {
	uint32_t padd = (bytes + 3) & ~3;
	struct svga_fifo_escape {
		uint32_t cmd;
		uint32_t nsid;
		uint32_t size;
	} __attribute__ ((packed));
	struct svga_fifo_escape* header = svga_fifo_reserve(padd + sizeof(struct svga_fifo_escape));
	header->cmd = SVGA_CMD_ESCAPE;
	header->nsid = nsid;
	header->size = bytes;
	return (void*)(header + 1);
}

// Commit the specified number of fifo commands
void svga_fifo_commit(uint32_t bytes) {
	uint32_t max = fifo_base[SVGA_FIFO_MAX];
	uint32_t min = fifo_base[SVGA_FIFO_MIN];
	uint32_t next_cmd = fifo_base[SVGA_FIFO_NEXT_CMD];
	
	if (bounce_buffer) {
		uint32_t size = (bytes < (max - next_cmd)) ? bytes : (max - next_cmd);
		fifo_base[SVGA_FIFO_RESERVED] = bytes;
		memcpy(next_cmd + (uint8_t*)fifo_base, bounce_buffer, size);
		memcpy(min + (uint8_t*)fifo_base, &bounce_buffer[size], bytes - size);
		kfree(bounce_buffer);
		bounce_buffer = NULL;
	}
	fifo_reserved_size = 0;
	next_cmd += bytes;
	if (next_cmd >= max)
		next_cmd -= max - min;
	fifo_base[SVGA_FIFO_NEXT_CMD] = next_cmd;
	fifo_base[SVGA_FIFO_RESERVED] = 0;
	
	// Locked in fifo reserve
	up(&fifo_lock);
}

// Commit all fifo commands
void svga_fifo_commit_all() {
	svga_fifo_commit(fifo_reserved_size);
}

// Update a region of the screen
void svga_update(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	SVGAFifoCmdUpdate* cmd = svga_fifo_reserve_cmd(SVGA_CMD_UPDATE, sizeof(SVGAFifoCmdUpdate));
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	svga_fifo_commit_all();
}

// Set the cursor image (premultiplied 32-bit BGRA image)
void svga_set_cursor_image(uint32_t hotspot_x, uint32_t hotspot_y, uint32_t width, uint32_t height, uint32_t* data) {
	if ((svga_read(SVGA_REG_CAPABILITIES) & SVGA_CAP_ALPHA_CURSOR)) {
		uint32_t size = width * height * sizeof(uint32_t);
		SVGAFifoCmdDefineAlphaCursor* cursor = svga_fifo_reserve_cmd(SVGA_CMD_DEFINE_ALPHA_CURSOR,
																	 sizeof(SVGAFifoCmdDefineAlphaCursor) + size);
		cursor->id = 0;
		cursor->hotspotX = hotspot_x;
		cursor->hotspotY = hotspot_y;
		cursor->width = width;
		cursor->height = height;
		memcpy((void*)(cursor + 1), data, size);
		svga_fifo_commit_all();
	} else {
		// Fall back to more complicated implementation
		uint32_t bits = 1;
		uint32_t alpha_row = ((width + 31) >> 5) << 2;
		uint32_t pix_row = ((width * bits + 31) >> 5) << 2;
		
		SVGAFifoCmdDefineCursor* cursor = svga_fifo_reserve_cmd(SVGA_CMD_DEFINE_CURSOR,
										sizeof(SVGAFifoCmdDefineCursor) + (alpha_row + pix_row) * height);
		cursor->id = 0;
		cursor->hotspotX = hotspot_x;
		cursor->hotspotY = hotspot_y;
		cursor->width = width;
		cursor->height = height;
		cursor->andMaskDepth = bits;
		cursor->xorMaskDepth = bits ;
		// Extract alpha bits in a mask
		uint32_t* alpha_mask = (uint32_t*)(cursor + 1);
		memset(alpha_mask, 0x00, alpha_row * height);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				uint32_t pos = y * width + x;
				uint32_t alpha = ((data[pos] >> 24) & 0xFF) < 0x7F;
				alpha_mask[pos / 32] |= (alpha << (31 - (pos % 32)));
			}
		}
		// Copy image data
		uint32_t* color = (uint32_t*)((uint8_t*)alpha_mask + alpha_row * height);
		memcpy(color, data, width * height * sizeof(uint32_t));
		svga_fifo_commit_all();
	}
}

// Set cursor position
void svga_set_cursor_position(uint32_t x, uint32_t y, bool visible) {
	fifo_base[SVGA_FIFO_CURSOR_X] = x;
	fifo_base[SVGA_FIFO_CURSOR_Y] = y;
	fifo_base[SVGA_FIFO_CURSOR_ON] = visible;
	fifo_base[SVGA_FIFO_CURSOR_COUNT]++;
}

// Insert a fence into the FIFO
uint32_t svga_fence_insert() {
	if (!(fifo_base[SVGA_FIFO_CAPABILITIES] & SVGA_FIFO_CAP_FENCE))
		return 1;
	
	struct svga_fence_cmd {
		uint32_t id;
		uint32_t fence;
	} __attribute__ ((packed));
	struct svga_fence_cmd* cmd;
	
	if (next_fence == 0 || next_fence >= 0x7FFFFFFF)
		next_fence = 1;
	uint32_t fence = next_fence++;
	
	cmd = svga_fifo_reserve(sizeof(struct svga_fence_cmd));
	cmd->id = SVGA_CMD_FENCE;
	cmd->fence = fence;
	svga_fifo_commit_all();
	
	return fence;
}

// Wait until this fence has been processed
void svga_fence_sync(uint32_t fence) {
	if (!(fifo_base[SVGA_FIFO_CAPABILITIES] & SVGA_FIFO_CAP_FENCE)) {
		svga_write(SVGA_REG_SYNC, 1);
		while (svga_read(SVGA_REG_BUSY))
			schedule();
		return;
	}
	
	bool busy = true;
	svga_write(SVGA_REG_SYNC, 1);
	while (!svga_fence_passed(fence) && busy) {
		schedule();
		busy = (svga_read(SVGA_REG_BUSY) != 0);
	}
}

// Returns true if this fence has already been processed
bool svga_fence_passed(uint32_t fence) {
	if (!fence || !(fifo_base[SVGA_FIFO_CAPABILITIES] & SVGA_FIFO_CAP_FENCE))
		return true;
	return ((int32_t)(fifo_base[SVGA_FIFO_FENCE] - fence)) >= 0;
}
