/**********************************************************
 * Copyright 1998-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/*
 * svga3d_reg.h --
 *
 *       SVGA 3D hardware definitions
 */

#ifndef _SVGA3D_REG_CPP_
#define _SVGA3D_REG_CPP_

#include <stdint.h>

/*
 * PCI device IDs.
 */
#define PCI_VENDOR_ID_VMWARE            0x15AD
#define PCI_DEVICE_ID_VMWARE_SVGA2      0x0405

/*
 * SVGA_REG_ENABLE bit definitions.
 */
#define SVGA_REG_ENABLE_DISABLE     0
#define SVGA_REG_ENABLE_ENABLE      1
#define SVGA_REG_ENABLE_HIDE        2
#define SVGA_REG_ENABLE_ENABLE_HIDE (SVGA_REG_ENABLE_ENABLE |\
SVGA_REG_ENABLE_HIDE)

/*
 * Legal values for the SVGA_REG_CURSOR_ON register in old-fashioned
 * cursor bypass mode. This is still supported, but no new guest
 * drivers should use it.
 */
#define SVGA_CURSOR_ON_HIDE            0x0   /* Must be 0 to maintain backward compatibility */
#define SVGA_CURSOR_ON_SHOW            0x1   /* Must be 1 to maintain backward compatibility */
#define SVGA_CURSOR_ON_REMOVE_FROM_FB  0x2   /* Remove the cursor from the framebuffer because we need to see what's under it */
#define SVGA_CURSOR_ON_RESTORE_TO_FB   0x3   /* Put the cursor back in the framebuffer so the user can see it */

/*
 * The maximum framebuffer size that can traced for e.g. guests in VESA mode.
 * The changeMap in the monitor is proportional to this number. Therefore, we'd
 * like to keep it as small as possible to reduce monitor overhead (using
 * SVGA_VRAM_MAX_SIZE for this increases the size of the shared area by over
 * 4k!).
 *
 * NB: For compatibility reasons, this value must be greater than 0xff0000.
 *     See bug 335072.
 */
#define SVGA_FB_MAX_TRACEABLE_SIZE      0x1000000

#define SVGA_MAX_PSEUDOCOLOR_DEPTH      8
#define SVGA_MAX_PSEUDOCOLORS           (1 << SVGA_MAX_PSEUDOCOLOR_DEPTH)
#define SVGA_NUM_PALETTE_REGS           (3 * SVGA_MAX_PSEUDOCOLORS)

#define SVGA_MAGIC         0x900000UL
#define SVGA_MAKE_ID(ver)  (SVGA_MAGIC << 8 | (ver))

/* Version 2 let the address of the frame buffer be unsigned on Win32 */
#define SVGA_VERSION_2     2
#define SVGA_ID_2          SVGA_MAKE_ID(SVGA_VERSION_2)

/* Version 1 has new registers starting with SVGA_REG_CAPABILITIES so
 PALETTE_BASE has moved */
#define SVGA_VERSION_1     1
#define SVGA_ID_1          SVGA_MAKE_ID(SVGA_VERSION_1)

/* Version 0 is the initial version */
#define SVGA_VERSION_0     0
#define SVGA_ID_0          SVGA_MAKE_ID(SVGA_VERSION_0)

/* "Invalid" value for all SVGA IDs. (Version ID, screen object ID, surface ID...) */
#define SVGA_ID_INVALID    0xFFFFFFFF

/* Port offsets, relative to BAR0 */
#define SVGA_INDEX_PORT         0x0
#define SVGA_VALUE_PORT         0x1
#define SVGA_BIOS_PORT          0x2
#define SVGA_IRQSTATUS_PORT     0x8

/*
 * Interrupt source flags for IRQSTATUS_PORT and IRQMASK.
 *
 * Interrupts are only supported when the
 * SVGA_CAP_IRQMASK capability is present.
 */
#define SVGA_IRQFLAG_ANY_FENCE            0x1    /* Any fence was passed */
#define SVGA_IRQFLAG_FIFO_PROGRESS        0x2    /* Made forward progress in the FIFO */
#define SVGA_IRQFLAG_FENCE_GOAL           0x4    /* SVGA_FIFO_FENCE_GOAL reached */

/*
 * Registers
 */

enum {
	SVGA_REG_ID = 0,
	SVGA_REG_ENABLE = 1,
	SVGA_REG_WIDTH = 2,
	SVGA_REG_HEIGHT = 3,
	SVGA_REG_MAX_WIDTH = 4,
	SVGA_REG_MAX_HEIGHT = 5,
	SVGA_REG_DEPTH = 6,
	SVGA_REG_BITS_PER_PIXEL = 7,       /* Current bpp in the guest */
	SVGA_REG_PSEUDOCOLOR = 8,
	SVGA_REG_RED_MASK = 9,
	SVGA_REG_GREEN_MASK = 10,
	SVGA_REG_BLUE_MASK = 11,
	SVGA_REG_BYTES_PER_LINE = 12,
	SVGA_REG_FB_START = 13,            /* (Deprecated) */
	SVGA_REG_FB_OFFSET = 14,
	SVGA_REG_VRAM_SIZE = 15,
	SVGA_REG_FB_SIZE = 16,
	
	/* ID 0 implementation only had the above registers, then the palette */
	
	SVGA_REG_CAPABILITIES = 17,
	SVGA_REG_MEM_START = 18,           /* (Deprecated) */
	SVGA_REG_MEM_SIZE = 19,
	SVGA_REG_CONFIG_DONE = 20,         /* Set when memory area configured */
	SVGA_REG_SYNC = 21,                /* See "FIFO Synchronization Registers" */
	SVGA_REG_BUSY = 22,                /* See "FIFO Synchronization Registers" */
	SVGA_REG_GUEST_ID = 23,            /* Set guest OS identifier */
	SVGA_REG_CURSOR_ID = 24,           /* (Deprecated) */
	SVGA_REG_CURSOR_X = 25,            /* (Deprecated) */
	SVGA_REG_CURSOR_Y = 26,            /* (Deprecated) */
	SVGA_REG_CURSOR_ON = 27,           /* (Deprecated) */
	SVGA_REG_HOST_BITS_PER_PIXEL = 28, /* (Deprecated) */
	SVGA_REG_SCRATCH_SIZE = 29,        /* Number of scratch registers */
	SVGA_REG_MEM_REGS = 30,            /* Number of FIFO registers */
	SVGA_REG_NUM_DISPLAYS = 31,        /* (Deprecated) */
	SVGA_REG_PITCHLOCK = 32,           /* Fixed pitch for all modes */
	SVGA_REG_IRQMASK = 33,             /* Interrupt mask */
	
	/* Legacy multi-monitor support */
	SVGA_REG_NUM_GUEST_DISPLAYS = 34,/* Number of guest displays in X/Y direction */
	SVGA_REG_DISPLAY_ID = 35,        /* Display ID for the following display attributes */
	SVGA_REG_DISPLAY_IS_PRIMARY = 36,/* Whether this is a primary display */
	SVGA_REG_DISPLAY_POSITION_X = 37,/* The display position x */
	SVGA_REG_DISPLAY_POSITION_Y = 38,/* The display position y */
	SVGA_REG_DISPLAY_WIDTH = 39,     /* The display's width */
	SVGA_REG_DISPLAY_HEIGHT = 40,    /* The display's height */
	
	/* See "Guest memory regions" below. */
	SVGA_REG_GMR_ID = 41,
	SVGA_REG_GMR_DESCRIPTOR = 42,
	SVGA_REG_GMR_MAX_IDS = 43,
	SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH = 44,
	
	SVGA_REG_TRACES = 45,            /* Enable trace-based updates even when FIFO is on */
	SVGA_REG_GMRS_MAX_PAGES = 46,    /* Maximum number of 4KB pages for all GMRs */
	SVGA_REG_MEMORY_SIZE = 47,       /* Total dedicated device memory excluding FIFO */
	SVGA_REG_TOP = 48,               /* Must be 1 more than the last register */
	
	SVGA_PALETTE_BASE = 1024,        /* Base of SVGA color map */
	/* Next 768 (== 256*3) registers exist for colormap */
	
	SVGA_SCRATCH_BASE = SVGA_PALETTE_BASE + SVGA_NUM_PALETTE_REGS
	/* Base of scratch registers */
	/* Next reg[SVGA_REG_SCRATCH_SIZE] registers exist for scratch usage:
	 First 4 are reserved for VESA BIOS Extension; any remaining are for
	 the use of the current SVGA driver. */
};


/*
 * Guest memory regions (GMRs):
 *
 * This is a new memory mapping feature available in SVGA devices
 * which have the SVGA_CAP_GMR bit set. Previously, there were two
 * fixed memory regions available with which to share data between the
 * device and the driver: the FIFO ('MEM') and the framebuffer. GMRs
 * are our name for an extensible way of providing arbitrary DMA
 * buffers for use between the driver and the SVGA device. They are a
 * new alternative to framebuffer memory, usable for both 2D and 3D
 * graphics operations.
 *
 * Since GMR mapping must be done synchronously with guest CPU
 * execution, we use a new pair of SVGA registers:
 *
 *   SVGA_REG_GMR_ID --
 *
 *     Read/write.
 *     This register holds the 32-bit ID (a small positive integer)
 *     of a GMR to create, delete, or redefine. Writing this register
 *     has no side-effects.
 *
 *   SVGA_REG_GMR_DESCRIPTOR --
 *
 *     Write-only.
 *     Writing this register will create, delete, or redefine the GMR
 *     specified by the above ID register. If this register is zero,
 *     the GMR is deleted. Any pointers into this GMR (including those
 *     currently being processed by FIFO commands) will be
 *     synchronously invalidated.
 *
 *     If this register is nonzero, it must be the physical page
 *     number (PPN) of a data structure which describes the physical
 *     layout of the memory region this GMR should describe. The
 *     descriptor structure will be read synchronously by the SVGA
 *     device when this register is written. The descriptor need not
 *     remain allocated for the lifetime of the GMR.
 *
 *     The guest driver should write SVGA_REG_GMR_ID first, then
 *     SVGA_REG_GMR_DESCRIPTOR.
 *
 *   SVGA_REG_GMR_MAX_IDS --
 *
 *     Read-only.
 *     The SVGA device may choose to support a maximum number of
 *     user-defined GMR IDs. This register holds the number of supported
 *     IDs. (The maximum supported ID plus 1)
 *
 *   SVGA_REG_GMR_MAX_DESCRIPTOR_LENGTH --
 *
 *     Read-only.
 *     The SVGA device may choose to put a limit on the total number
 *     of SVGAGuestMemDescriptor structures it will read when defining
 *     a single GMR.
 *
 * The descriptor structure is an array of SVGAGuestMemDescriptor
 * structures. Each structure may do one of three things:
 *
 *   - Terminate the GMR descriptor list.
 *     (ppn==0, numPages==0)
 *
 *   - Add a PPN or range of PPNs to the GMR's virtual address space.
 *     (ppn != 0, numPages != 0)
 *
 *   - Provide the PPN of the next SVGAGuestMemDescriptor, in order to
 *     support multi-page GMR descriptor tables without forcing the
 *     driver to allocate physically contiguous memory.
 *     (ppn != 0, numPages == 0)
 *
 * Note that each physical page of SVGAGuestMemDescriptor structures
 * can describe at least 2MB of guest memory. If the driver needs to
 * use more than one page of descriptor structures, it must use one of
 * its SVGAGuestMemDescriptors to point to an additional page.  The
 * device will never automatically cross a page boundary.
 *
 * Once the driver has described a GMR, it is immediately available
 * for use via any FIFO command that uses an SVGAGuestPtr structure.
 * These pointers include a GMR identifier plus an offset into that
 * GMR.
 *
 * The driver must check the SVGA_CAP_GMR bit before using the GMR
 * registers.
 */

/*
 * Special GMR IDs, allowing SVGAGuestPtrs to point to framebuffer
 * memory as well.  In the future, these IDs could even be used to
 * allow legacy memory regions to be redefined by the guest as GMRs.
 *
 * Using the guest framebuffer (GFB) at BAR1 for general purpose DMA
 * is being phased out. Please try to use user-defined GMRs whenever
 * possible.
 */
#define SVGA_GMR_NULL         ((uint32_t) -1)
#define SVGA_GMR_FRAMEBUFFER  ((uint32_t) -2)  // Guest Framebuffer (GFB)

typedef
struct SVGAGuestMemDescriptor {
	uint32_t ppn;
	uint32_t numPages;
} SVGAGuestMemDescriptor;

typedef
struct SVGAGuestPtr {
	uint32_t gmrId;
	uint32_t offset;
} SVGAGuestPtr;


/*
 * SVGAGMRImageFormat --
 *
 *    This is a packed representation of the source 2D image format
 *    for a GMR-to-screen blit. Currently it is defined as an encoding
 *    of the screen's color depth and bits-per-pixel, however, 16 bits
 *    are reserved for future use to identify other encodings (such as
 *    RGBA or higher-precision images).
 *
 *    Currently supported formats:
 *
 *       bpp depth  Format Name
 *       --- -----  -----------
 *        32    24  32-bit BGRX
 *        24    24  24-bit BGR
 *        16    16  RGB 5-6-5
 *        16    15  RGB 5-5-5
 *
 */

typedef
struct SVGAGMRImageFormat {
	union {
		struct {
			uint32_t bitsPerPixel : 8;
			uint32_t colorDepth   : 8;
			uint32_t reserved     : 16;  // Must be zero
		};
		
		uint32_t value;
	};
} SVGAGMRImageFormat;

typedef
struct SVGAGuestImage {
	SVGAGuestPtr         ptr;
	
	/*
	 * A note on interpretation of pitch: This value of pitch is the
	 * number of bytes between vertically adjacent image
	 * blocks. Normally this is the number of bytes between the first
	 * pixel of two adjacent scanlines. With compressed textures,
	 * however, this may represent the number of bytes between
	 * compression blocks rather than between rows of pixels.
	 *
	 * XXX: Compressed textures currently must be tightly packed in guest memory.
	 *
	 * If the image is 1-dimensional, pitch is ignored.
	 *
	 * If 'pitch' is zero, the SVGA3D device calculates a pitch value
	 * assuming each row of blocks is tightly packed.
	 */
	uint32_t pitch;
} SVGAGuestImage;

/*
 * SVGAColorBGRX --
 *
 *    A 24-bit color format (BGRX), which does not depend on the
 *    format of the legacy guest framebuffer (GFB) or the current
 *    GMRFB state.
 */

typedef
struct SVGAColorBGRX {
	union {
		struct {
			uint32_t b : 8;
			uint32_t g : 8;
			uint32_t r : 8;
			uint32_t x : 8;  // Unused
		};
		
		uint32_t value;
	};
} SVGAColorBGRX;


/*
 * SVGASignedRect --
 * SVGASignedPoint --
 *
 *    Signed rectangle and point primitives. These are used by the new
 *    2D primitives for drawing to Screen Objects, which can occupy a
 *    signed virtual coordinate space.
 *
 *    SVGASignedRect specifies a half-open interval: the (left, top)
 *    pixel is part of the rectangle, but the (right, bottom) pixel is
 *    not.
 */

typedef
struct SVGASignedRect {
	int32_t  left;
	int32_t  top;
	int32_t  right;
	int32_t  bottom;
} SVGASignedRect;

typedef
struct SVGASignedPoint {
	int32_t  x;
	int32_t  y;
} SVGASignedPoint;


/*
 *  Capabilities
 *
 *  Note the holes in the bitfield. Missing bits have been deprecated,
 *  and must not be reused. Those capabilities will never be reported
 *  by new versions of the SVGA device.
 *
 * SVGA_CAP_GMR2 --
 *    Provides asynchronous commands to define and remap guest memory
 *    regions.  Adds device registers SVGA_REG_GMRS_MAX_PAGES and
 *    SVGA_REG_MEMORY_SIZE.
 *
 * SVGA_CAP_SCREEN_OBJECT_2 --
 *    Allow screen object support, and require backing stores from the
 *    guest for each screen object.
 */

#define SVGA_CAP_NONE               0x00000000
#define SVGA_CAP_RECT_COPY          0x00000002
#define SVGA_CAP_CURSOR             0x00000020
#define SVGA_CAP_CURSOR_BYPASS      0x00000040   // Legacy (Use Cursor Bypass 3 instead)
#define SVGA_CAP_CURSOR_BYPASS_2    0x00000080   // Legacy (Use Cursor Bypass 3 instead)
#define SVGA_CAP_8BIT_EMULATION     0x00000100
#define SVGA_CAP_ALPHA_CURSOR       0x00000200
#define SVGA_CAP_3D                 0x00004000
#define SVGA_CAP_EXTENDED_FIFO      0x00008000
#define SVGA_CAP_MULTIMON           0x00010000   // Legacy multi-monitor support
#define SVGA_CAP_PITCHLOCK          0x00020000
#define SVGA_CAP_IRQMASK            0x00040000
#define SVGA_CAP_DISPLAY_TOPOLOGY   0x00080000   // Legacy multi-monitor support
#define SVGA_CAP_GMR                0x00100000
#define SVGA_CAP_TRACES             0x00200000
#define SVGA_CAP_GMR2               0x00400000
#define SVGA_CAP_SCREEN_OBJECT_2    0x00800000


/*
 * FIFO register indices.
 *
 * The FIFO is a chunk of device memory mapped into guest physmem.  It
 * is always treated as 32-bit words.
 *
 * The guest driver gets to decide how to partition it between
 * - FIFO registers (there are always at least 4, specifying where the
 *   following data area is and how much data it contains; there may be
 *   more registers following these, depending on the FIFO protocol
 *   version in use)
 * - FIFO data, written by the guest and slurped out by the VMX.
 * These indices are 32-bit word offsets into the FIFO.
 */

enum {
	/*
	 * Block 1 (basic registers): The originally defined FIFO registers.
	 * These exist and are valid for all versions of the FIFO protocol.
	 */
	
	SVGA_FIFO_MIN = 0,
	SVGA_FIFO_MAX,       /* The distance from MIN to MAX must be at least 10K */
	SVGA_FIFO_NEXT_CMD,
	SVGA_FIFO_STOP,
	
	/*
	 * Block 2 (extended registers): Mandatory registers for the extended
	 * FIFO.  These exist if the SVGA caps register includes
	 * SVGA_CAP_EXTENDED_FIFO; some of them are valid only if their
	 * associated capability bit is enabled.
	 *
	 * Note that when originally defined, SVGA_CAP_EXTENDED_FIFO implied
	 * support only for (FIFO registers) CAPABILITIES, FLAGS, and FENCE.
	 * This means that the guest has to test individually (in most cases
	 * using FIFO caps) for the presence of registers after this; the VMX
	 * can define "extended FIFO" to mean whatever it wants, and currently
	 * won't enable it unless there's room for that set and much more.
	 */
	
	SVGA_FIFO_CAPABILITIES = 4,
	SVGA_FIFO_FLAGS,
	// Valid with SVGA_FIFO_CAP_FENCE:
	SVGA_FIFO_FENCE,
	
	/*
	 * Block 3a (optional extended registers): Additional registers for the
	 * extended FIFO, whose presence isn't actually implied by
	 * SVGA_CAP_EXTENDED_FIFO; these exist if SVGA_FIFO_MIN is high enough to
	 * leave room for them.
	 *
	 * These in block 3a, the VMX currently considers mandatory for the
	 * extended FIFO.
	 */
	
	// Valid if exists (i.e. if extended FIFO enabled):
	SVGA_FIFO_3D_HWVERSION,       /* See SVGA3dHardwareVersion in svga3d_reg.h */
	// Valid with SVGA_FIFO_CAP_PITCHLOCK:
	SVGA_FIFO_PITCHLOCK,
	
	// Valid with SVGA_FIFO_CAP_CURSOR_BYPASS_3:
	SVGA_FIFO_CURSOR_ON,          /* Cursor bypass 3 show/hide register */
	SVGA_FIFO_CURSOR_X,           /* Cursor bypass 3 x register */
	SVGA_FIFO_CURSOR_Y,           /* Cursor bypass 3 y register */
	SVGA_FIFO_CURSOR_COUNT,       /* Incremented when any of the other 3 change */
	SVGA_FIFO_CURSOR_LAST_UPDATED,/* Last time the host updated the cursor */
	
	// Valid with SVGA_FIFO_CAP_RESERVE:
	SVGA_FIFO_RESERVED,           /* Bytes past NEXT_CMD with real contents */
	
	/*
	 * Valid with SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2:
	 *
	 * By default this is SVGA_ID_INVALID, to indicate that the cursor
	 * coordinates are specified relative to the virtual root. If this
	 * is set to a specific screen ID, cursor position is reinterpreted
	 * as a signed offset relative to that screen's origin.
	 */
	SVGA_FIFO_CURSOR_SCREEN_ID,
	
	/*
	 * Valid with SVGA_FIFO_CAP_DEAD
	 *
	 * An arbitrary value written by the host, drivers should not use it.
	 */
	SVGA_FIFO_DEAD,
	
	/*
	 * Valid with SVGA_FIFO_CAP_3D_HWVERSION_REVISED:
	 *
	 * Contains 3D HWVERSION (see SVGA3dHardwareVersion in svga3d_reg.h)
	 * on platforms that can enforce graphics resource limits.
	 */
	SVGA_FIFO_3D_HWVERSION_REVISED,
	
	/*
	 * XXX: The gap here, up until SVGA_FIFO_3D_CAPS, can be used for new
	 * registers, but this must be done carefully and with judicious use of
	 * capability bits, since comparisons based on SVGA_FIFO_MIN aren't
	 * enough to tell you whether the register exists: we've shipped drivers
	 * and products that used SVGA_FIFO_3D_CAPS but didn't know about some of
	 * the earlier ones.  The actual order of introduction was:
	 * - PITCHLOCK
	 * - 3D_CAPS
	 * - CURSOR_* (cursor bypass 3)
	 * - RESERVED
	 * So, code that wants to know whether it can use any of the
	 * aforementioned registers, or anything else added after PITCHLOCK and
	 * before 3D_CAPS, needs to reason about something other than
	 * SVGA_FIFO_MIN.
	 */
	
	/*
	 * 3D caps block space; valid with 3D hardware version >=
	 * SVGA3D_HWVERSION_WS6_B1.
	 */
	SVGA_FIFO_3D_CAPS      = 32,
	SVGA_FIFO_3D_CAPS_LAST = 32 + 255,
	
	/*
	 * End of VMX's current definition of "extended-FIFO registers".
	 * Registers before here are always enabled/disabled as a block; either
	 * the extended FIFO is enabled and includes all preceding registers, or
	 * it's disabled entirely.
	 *
	 * Block 3b (truly optional extended registers): Additional registers for
	 * the extended FIFO, which the VMX already knows how to enable and
	 * disable with correct granularity.
	 *
	 * Registers after here exist if and only if the guest SVGA driver
	 * sets SVGA_FIFO_MIN high enough to leave room for them.
	 */
	
	// Valid if register exists:
	SVGA_FIFO_GUEST_3D_HWVERSION, /* Guest driver's 3D version */
	SVGA_FIFO_FENCE_GOAL,         /* Matching target for SVGA_IRQFLAG_FENCE_GOAL */
	SVGA_FIFO_BUSY,               /* See "FIFO Synchronization Registers" */
	
	/*
	 * Always keep this last.  This defines the maximum number of
	 * registers we know about.  At power-on, this value is placed in
	 * the SVGA_REG_MEM_REGS register, and we expect the guest driver
	 * to allocate this much space in FIFO memory for registers.
	 */
	SVGA_FIFO_NUM_REGS
};


/*
 * Definition of registers included in extended FIFO support.
 *
 * The guest SVGA driver gets to allocate the FIFO between registers
 * and data.  It must always allocate at least 4 registers, but old
 * drivers stopped there.
 *
 * The VMX will enable extended FIFO support if and only if the guest
 * left enough room for all registers defined as part of the mandatory
 * set for the extended FIFO.
 *
 * Note that the guest drivers typically allocate the FIFO only at
 * initialization time, not at mode switches, so it's likely that the
 * number of FIFO registers won't change without a reboot.
 *
 * All registers less than this value are guaranteed to be present if
 * svgaUser->fifo.extended is set. Any later registers must be tested
 * individually for compatibility at each use (in the VMX).
 *
 * This value is used only by the VMX, so it can change without
 * affecting driver compatibility; keep it that way?
 */
#define SVGA_FIFO_EXTENDED_MANDATORY_REGS  (SVGA_FIFO_3D_CAPS_LAST + 1)


/*
 * FIFO Synchronization Registers
 *
 *  This explains the relationship between the various FIFO
 *  sync-related registers in IOSpace and in FIFO space.
 *
 *  SVGA_REG_SYNC --
 *
 *       The SYNC register can be used in two different ways by the guest:
 *
 *         1. If the guest wishes to fully sync (drain) the FIFO,
 *            it will write once to SYNC then poll on the BUSY
 *            register. The FIFO is sync'ed once BUSY is zero.
 *
 *         2. If the guest wants to asynchronously wake up the host,
 *            it will write once to SYNC without polling on BUSY.
 *            Ideally it will do this after some new commands have
 *            been placed in the FIFO, and after reading a zero
 *            from SVGA_FIFO_BUSY.
 *
 *       (1) is the original behaviour that SYNC was designed to
 *       support.  Originally, a write to SYNC would implicitly
 *       trigger a read from BUSY. This causes us to synchronously
 *       process the FIFO.
 *
 *       This behaviour has since been changed so that writing SYNC
 *       will *not* implicitly cause a read from BUSY. Instead, it
 *       makes a channel call which asynchronously wakes up the MKS
 *       thread.
 *
 *       New guests can use this new behaviour to implement (2)
 *       efficiently. This lets guests get the host's attention
 *       without waiting for the MKS to poll, which gives us much
 *       better CPU utilization on SMP hosts and on UP hosts while
 *       we're blocked on the host GPU.
 *
 *       Old guests shouldn't notice the behaviour change. SYNC was
 *       never guaranteed to process the entire FIFO, since it was
 *       bounded to a particular number of CPU cycles. Old guests will
 *       still loop on the BUSY register until the FIFO is empty.
 *
 *       Writing to SYNC currently has the following side-effects:
 *
 *         - Sets SVGA_REG_BUSY to TRUE (in the monitor)
 *         - Asynchronously wakes up the MKS thread for FIFO processing
 *         - The value written to SYNC is recorded as a "reason", for
 *           stats purposes.
 *
 *       If SVGA_FIFO_BUSY is available, drivers are advised to only
 *       write to SYNC if SVGA_FIFO_BUSY is FALSE. Drivers should set
 *       SVGA_FIFO_BUSY to TRUE after writing to SYNC. The MKS will
 *       eventually set SVGA_FIFO_BUSY on its own, but this approach
 *       lets the driver avoid sending multiple asynchronous wakeup
 *       messages to the MKS thread.
 *
 *  SVGA_REG_BUSY --
 *
 *       This register is set to TRUE when SVGA_REG_SYNC is written,
 *       and it reads as FALSE when the FIFO has been completely
 *       drained.
 *
 *       Every read from this register causes us to synchronously
 *       process FIFO commands. There is no guarantee as to how many
 *       commands each read will process.
 *
 *       CPU time spent processing FIFO commands will be billed to
 *       the guest.
 *
 *       New drivers should avoid using this register unless they
 *       need to guarantee that the FIFO is completely drained. It
 *       is overkill for performing a sync-to-fence. Older drivers
 *       will use this register for any type of synchronization.
 *
 *  SVGA_FIFO_BUSY --
 *
 *       This register is a fast way for the guest driver to check
 *       whether the FIFO is already being processed. It reads and
 *       writes at normal RAM speeds, with no monitor intervention.
 *
 *       If this register reads as TRUE, the host is guaranteeing that
 *       any new commands written into the FIFO will be noticed before
 *       the MKS goes back to sleep.
 *
 *       If this register reads as FALSE, no such guarantee can be
 *       made.
 *
 *       The guest should use this register to quickly determine
 *       whether or not it needs to wake up the host. If the guest
 *       just wrote a command or group of commands that it would like
 *       the host to begin processing, it should:
 *
 *         1. Read SVGA_FIFO_BUSY. If it reads as TRUE, no further
 *            action is necessary.
 *
 *         2. Write TRUE to SVGA_FIFO_BUSY. This informs future guest
 *            code that we've already sent a SYNC to the host and we
 *            don't need to send a duplicate.
 *
 *         3. Write a reason to SVGA_REG_SYNC. This will send an
 *            asynchronous wakeup to the MKS thread.
 */


/*
 * FIFO Capabilities
 *
 *      Fence -- Fence register and command are supported
 *      Accel Front -- Front buffer only commands are supported
 *      Pitch Lock -- Pitch lock register is supported
 *      Video -- SVGA Video overlay units are supported
 *      Escape -- Escape command is supported
 *
 * XXX: Add longer descriptions for each capability, including a list
 *      of the new features that each capability provides.
 *
 * SVGA_FIFO_CAP_SCREEN_OBJECT --
 *
 *    Provides dynamic multi-screen rendering, for improved Unity and
 *    multi-monitor modes. With Screen Object, the guest can
 *    dynamically create and destroy 'screens', which can represent
 *    Unity windows or virtual monitors. Screen Object also provides
 *    strong guarantees that DMA operations happen only when
 *    guest-initiated. Screen Object deprecates the BAR1 guest
 *    framebuffer (GFB) and all commands that work only with the GFB.
 *
 *    New registers:
 *       FIFO_CURSOR_SCREEN_ID, VIDEO_DATA_GMRID, VIDEO_DST_SCREEN_ID
 *
 *    New 2D commands:
 *       DEFINE_SCREEN, DESTROY_SCREEN, DEFINE_GMRFB, BLIT_GMRFB_TO_SCREEN,
 *       BLIT_SCREEN_TO_GMRFB, ANNOTATION_FILL, ANNOTATION_COPY
 *
 *    New 3D commands:
 *       BLIT_SURFACE_TO_SCREEN
 *
 *    New guarantees:
 *
 *       - The host will not read or write guest memory, including the GFB,
 *         except when explicitly initiated by a DMA command.
 *
 *       - All DMA, including legacy DMA like UPDATE and PRESENT_READBACK,
 *         is guaranteed to complete before any subsequent FENCEs.
 *
 *       - All legacy commands which affect a Screen (UPDATE, PRESENT,
 *         PRESENT_READBACK) as well as new Screen blit commands will
 *         all behave consistently as blits, and memory will be read
 *         or written in FIFO order.
 *
 *         For example, if you PRESENT from one SVGA3D surface to multiple
 *         places on the screen, the data copied will always be from the
 *         SVGA3D surface at the time the PRESENT was issued in the FIFO.
 *         This was not necessarily true on devices without Screen Object.
 *
 *         This means that on devices that support Screen Object, the
 *         PRESENT_READBACK command should not be necessary unless you
 *         actually want to read back the results of 3D rendering into
 *         system memory. (And for that, the BLIT_SCREEN_TO_GMRFB
 *         command provides a strict superset of functionality.)
 *
 *       - When a screen is resized, either using Screen Object commands or
 *         legacy multimon registers, its contents are preserved.
 *
 * SVGA_FIFO_CAP_GMR2 --
 *
 *    Provides new commands to define and remap guest memory regions (GMR).
 *
 *    New 2D commands:
 *       DEFINE_GMR2, REMAP_GMR2.
 *
 * SVGA_FIFO_CAP_3D_HWVERSION_REVISED --
 *
 *    Indicates new register SVGA_FIFO_3D_HWVERSION_REVISED exists.
 *    This register may replace SVGA_FIFO_3D_HWVERSION on platforms
 *    that enforce graphics resource limits.  This allows the platform
 *    to clear SVGA_FIFO_3D_HWVERSION and disable 3D in legacy guest
 *    drivers that do not limit their resources.
 *
 *    Note this is an alias to SVGA_FIFO_CAP_GMR2 because these indicators
 *    are codependent (and thus we use a single capability bit).
 *
 * SVGA_FIFO_CAP_SCREEN_OBJECT_2 --
 *
 *    Modifies the DEFINE_SCREEN command to include a guest provided
 *    backing store in GMR memory and the bytesPerLine for the backing
 *    store.  This capability requires the use of a backing store when
 *    creating screen objects.  However if SVGA_FIFO_CAP_SCREEN_OBJECT
 *    is present then backing stores are optional.
 *
 * SVGA_FIFO_CAP_DEAD --
 *
 *    Drivers should not use this cap bit.  This cap bit can not be
 *    reused since some hosts already expose it.
 */

#define SVGA_FIFO_CAP_NONE                  0
#define SVGA_FIFO_CAP_FENCE             (1<<0)
#define SVGA_FIFO_CAP_ACCELFRONT        (1<<1)
#define SVGA_FIFO_CAP_PITCHLOCK         (1<<2)
#define SVGA_FIFO_CAP_VIDEO             (1<<3)
#define SVGA_FIFO_CAP_CURSOR_BYPASS_3   (1<<4)
#define SVGA_FIFO_CAP_ESCAPE            (1<<5)
#define SVGA_FIFO_CAP_RESERVE           (1<<6)
#define SVGA_FIFO_CAP_SCREEN_OBJECT     (1<<7)
#define SVGA_FIFO_CAP_GMR2              (1<<8)
#define SVGA_FIFO_CAP_3D_HWVERSION_REVISED  SVGA_FIFO_CAP_GMR2
#define SVGA_FIFO_CAP_SCREEN_OBJECT_2   (1<<9)
#define SVGA_FIFO_CAP_DEAD              (1<<10)


/*
 * FIFO Flags
 *
 *      Accel Front -- Driver should use front buffer only commands
 */

#define SVGA_FIFO_FLAG_NONE                 0
#define SVGA_FIFO_FLAG_ACCELFRONT       (1<<0)
#define SVGA_FIFO_FLAG_RESERVED        (1<<31) // Internal use only

/*
 * FIFO reservation sentinel value
 */

#define SVGA_FIFO_RESERVED_UNKNOWN      0xffffffff


/*
 * Video overlay support
 */

#define SVGA_NUM_OVERLAY_UNITS 32


/*
 * Video capabilities that the guest is currently using
 */

#define SVGA_VIDEO_FLAG_COLORKEY        0x0001


/*
 * Offsets for the video overlay registers
 */

enum {
	SVGA_VIDEO_ENABLED = 0,
	SVGA_VIDEO_FLAGS,
	SVGA_VIDEO_DATA_OFFSET,
	SVGA_VIDEO_FORMAT,
	SVGA_VIDEO_COLORKEY,
	SVGA_VIDEO_SIZE,          // Deprecated
	SVGA_VIDEO_WIDTH,
	SVGA_VIDEO_HEIGHT,
	SVGA_VIDEO_SRC_X,
	SVGA_VIDEO_SRC_Y,
	SVGA_VIDEO_SRC_WIDTH,
	SVGA_VIDEO_SRC_HEIGHT,
	SVGA_VIDEO_DST_X,         // Signed int32_t
	SVGA_VIDEO_DST_Y,         // Signed int32_t
	SVGA_VIDEO_DST_WIDTH,
	SVGA_VIDEO_DST_HEIGHT,
	SVGA_VIDEO_PITCH_1,
	SVGA_VIDEO_PITCH_2,
	SVGA_VIDEO_PITCH_3,
	SVGA_VIDEO_DATA_GMRID,    // Optional, defaults to SVGA_GMR_FRAMEBUFFER
	SVGA_VIDEO_DST_SCREEN_ID, // Optional, defaults to virtual coords (SVGA_ID_INVALID)
	SVGA_VIDEO_NUM_REGS
};


/*
 * SVGA Overlay Units
 *
 *      width and height relate to the entire source video frame.
 *      srcX, srcY, srcWidth and srcHeight represent subset of the source
 *      video frame to be displayed.
 */

typedef struct SVGAOverlayUnit {
	uint32_t enabled;
	uint32_t flags;
	uint32_t dataOffset;
	uint32_t format;
	uint32_t colorKey;
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint32_t srcX;
	uint32_t srcY;
	uint32_t srcWidth;
	uint32_t srcHeight;
	int32_t  dstX;
	int32_t  dstY;
	uint32_t dstWidth;
	uint32_t dstHeight;
	uint32_t pitches[3];
	uint32_t dataGMRId;
	uint32_t dstScreenId;
} SVGAOverlayUnit;


/*
 * SVGAScreenObject --
 *
 *    This is a new way to represent a guest's multi-monitor screen or
 *    Unity window. Screen objects are only supported if the
 *    SVGA_FIFO_CAP_SCREEN_OBJECT capability bit is set.
 *
 *    If Screen Objects are supported, they can be used to fully
 *    replace the functionality provided by the framebuffer registers
 *    (SVGA_REG_WIDTH, HEIGHT, etc.) and by SVGA_CAP_DISPLAY_TOPOLOGY.
 *
 *    The screen object is a struct with guaranteed binary
 *    compatibility. New flags can be added, and the struct may grow,
 *    but existing fields must retain their meaning.
 *
 *    Added with SVGA_FIFO_CAP_SCREEN_OBJECT_2 are required fields of
 *    a SVGAGuestPtr that is used to back the screen contents.  This
 *    memory must come from the GFB.  The guest is not allowed to
 *    access the memory and doing so will have undefined results.  The
 *    backing store is required to be page aligned and the size is
 *    padded to the next page boundry.  The number of pages is:
 *       (bytesPerLine * size.width * 4 + PAGE_SIZE - 1) / PAGE_SIZE
 *
 *    The pitch in the backingStore is required to be at least large
 *    enough to hold a 32bbp scanline.  It is recommended that the
 *    driver pad bytesPerLine for a potential performance win.
 *
 *    The cloneCount field is treated as a hint from the guest that
 *    the user wants this display to be cloned, countCount times.  A
 *    value of zero means no cloning should happen.
 */

#define SVGA_SCREEN_MUST_BE_SET     (1 << 0) // Must be set or results undefined
#define SVGA_SCREEN_HAS_ROOT SVGA_SCREEN_MUST_BE_SET // Deprecated
#define SVGA_SCREEN_IS_PRIMARY      (1 << 1) // Guest considers this screen to be 'primary'
#define SVGA_SCREEN_FULLSCREEN_HINT (1 << 2)  // Guest is running a fullscreen app here

/*
 * Added with SVGA_FIFO_CAP_SCREEN_OBJECT_2.  When the screen is
 * deactivated the base layer is defined to lose all contents and
 * become black.  When a screen is deactivated the backing store is
 * optional.  When set backingPtr and bytesPerLine will be ignored.
 */
#define SVGA_SCREEN_DEACTIVATE  (1 << 3)

/*
 * Added with SVGA_FIFO_CAP_SCREEN_OBJECT_2.  When this flag is set
 * the screen contents will be outputted as all black to the user
 * though the base layer contents is preserved.  The screen base layer
 * can still be read and written to like normal though the no visible
 * effect will be seen by the user.  When the flag is changed the
 * screen will be blanked or redrawn to the current contents as needed
 * without any extra commands from the driver.  This flag only has an
 * effect when the screen is not deactivated.
 */
#define SVGA_SCREEN_BLANKING (1 << 4)

typedef
struct SVGAScreenObject {
	uint32_t structSize;   // sizeof(SVGAScreenObject)
	uint32_t id;
	uint32_t flags;
	struct {
		uint32_t width;
		uint32_t height;
	} size;
	struct {
		int32_t x;
		int32_t y;
	} root;
	
	/*
	 * Added and required by SVGA_FIFO_CAP_SCREEN_OBJECT_2, optional
	 * with SVGA_FIFO_CAP_SCREEN_OBJECT.
	 */
	SVGAGuestImage backingStore;
	uint32_t cloneCount;
} SVGAScreenObject;


/*
 *  Commands in the command FIFO:
 *
 *  Command IDs defined below are used for the traditional 2D FIFO
 *  communication (not all commands are available for all versions of the
 *  SVGA FIFO protocol).
 *
 *  Note the holes in the command ID numbers: These commands have been
 *  deprecated, and the old IDs must not be reused.
 *
 *  Command IDs from 1000 to 1999 are reserved for use by the SVGA3D
 *  protocol.
 *
 *  Each command's parameters are described by the comments and
 *  structs below.
 */

typedef enum {
	SVGA_CMD_INVALID_CMD           = 0,
	SVGA_CMD_UPDATE                = 1,
	SVGA_CMD_RECT_COPY             = 3,
	SVGA_CMD_DEFINE_CURSOR         = 19,
	SVGA_CMD_DEFINE_ALPHA_CURSOR   = 22,
	SVGA_CMD_UPDATE_VERBOSE        = 25,
	SVGA_CMD_FRONT_ROP_FILL        = 29,
	SVGA_CMD_FENCE                 = 30,
	SVGA_CMD_ESCAPE                = 33,
	SVGA_CMD_DEFINE_SCREEN         = 34,
	SVGA_CMD_DESTROY_SCREEN        = 35,
	SVGA_CMD_DEFINE_GMRFB          = 36,
	SVGA_CMD_BLIT_GMRFB_TO_SCREEN  = 37,
	SVGA_CMD_BLIT_SCREEN_TO_GMRFB  = 38,
	SVGA_CMD_ANNOTATION_FILL       = 39,
	SVGA_CMD_ANNOTATION_COPY       = 40,
	SVGA_CMD_DEFINE_GMR2           = 41,
	SVGA_CMD_REMAP_GMR2            = 42,
	SVGA_CMD_MAX
} SVGAFifoCmdId;

#define SVGA_CMD_MAX_DATASIZE       (256 * 1024)
#define SVGA_CMD_MAX_ARGS           64


/*
 * SVGA_CMD_UPDATE --
 *
 *    This is a DMA transfer which copies from the Guest Framebuffer
 *    (GFB) at BAR1 + SVGA_REG_FB_OFFSET to any screens which
 *    intersect with the provided virtual rectangle.
 *
 *    This command does not support using arbitrary guest memory as a
 *    data source- it only works with the pre-defined GFB memory.
 *    This command also does not support signed virtual coordinates.
 *    If you have defined screens (using SVGA_CMD_DEFINE_SCREEN) with
 *    negative root x/y coordinates, the negative portion of those
 *    screens will not be reachable by this command.
 *
 *    This command is not necessary when using framebuffer
 *    traces. Traces are automatically enabled if the SVGA FIFO is
 *    disabled, and you may explicitly enable/disable traces using
 *    SVGA_REG_TRACES. With traces enabled, any write to the GFB will
 *    automatically act as if a subsequent SVGA_CMD_UPDATE was issued.
 *
 *    Traces and SVGA_CMD_UPDATE are the only supported ways to render
 *    pseudocolor screen updates. The newer Screen Object commands
 *    only support true color formats.
 *
 * Availability:
 *    Always available.
 */

typedef
struct {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
} SVGAFifoCmdUpdate;


/*
 * SVGA_CMD_RECT_COPY --
 *
 *    Perform a rectangular DMA transfer from one area of the GFB to
 *    another, and copy the result to any screens which intersect it.
 *
 * Availability:
 *    SVGA_CAP_RECT_COPY
 */

typedef
struct {
	uint32_t srcX;
	uint32_t srcY;
	uint32_t destX;
	uint32_t destY;
	uint32_t width;
	uint32_t height;
} SVGAFifoCmdRectCopy;


/*
 * SVGA_CMD_DEFINE_CURSOR --
 *
 *    Provide a new cursor image, as an AND/XOR mask.
 *
 *    The recommended way to position the cursor overlay is by using
 *    the SVGA_FIFO_CURSOR_* registers, supported by the
 *    SVGA_FIFO_CAP_CURSOR_BYPASS_3 capability.
 *
 * Availability:
 *    SVGA_CAP_CURSOR
 */

typedef
struct {
	uint32_t id;             // Reserved, must be zero.
	uint32_t hotspotX;
	uint32_t hotspotY;
	uint32_t width;
	uint32_t height;
	uint32_t andMaskDepth;   // Value must be 1 or equal to BITS_PER_PIXEL
	uint32_t xorMaskDepth;   // Value must be 1 or equal to BITS_PER_PIXEL
	/*
	 * Followed by scanline data for AND mask, then XOR mask.
	 * Each scanline is padded to a 32-bit boundary.
	 */
} SVGAFifoCmdDefineCursor;


/*
 * SVGA_CMD_DEFINE_ALPHA_CURSOR --
 *
 *    Provide a new cursor image, in 32-bit BGRA format.
 *
 *    The recommended way to position the cursor overlay is by using
 *    the SVGA_FIFO_CURSOR_* registers, supported by the
 *    SVGA_FIFO_CAP_CURSOR_BYPASS_3 capability.
 *
 * Availability:
 *    SVGA_CAP_ALPHA_CURSOR
 */

typedef
struct {
	uint32_t id;             // Reserved, must be zero.
	uint32_t hotspotX;
	uint32_t hotspotY;
	uint32_t width;
	uint32_t height;
	/* Followed by scanline data */
} SVGAFifoCmdDefineAlphaCursor;


/*
 * SVGA_CMD_UPDATE_VERBOSE --
 *
 *    Just like SVGA_CMD_UPDATE, but also provide a per-rectangle
 *    'reason' value, an opaque cookie which is used by internal
 *    debugging tools. Third party drivers should not use this
 *    command.
 *
 * Availability:
 *    SVGA_CAP_EXTENDED_FIFO
 */

typedef
struct {
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t reason;
} SVGAFifoCmdUpdateVerbose;


/*
 * SVGA_CMD_FRONT_ROP_FILL --
 *
 *    This is a hint which tells the SVGA device that the driver has
 *    just filled a rectangular region of the GFB with a solid
 *    color. Instead of reading these pixels from the GFB, the device
 *    can assume that they all equal 'color'. This is primarily used
 *    for remote desktop protocols.
 *
 * Availability:
 *    SVGA_FIFO_CAP_ACCELFRONT
 */

#define  SVGA_ROP_COPY                    0x03

typedef
struct {
	uint32_t color;     // In the same format as the GFB
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
	uint32_t rop;       // Must be SVGA_ROP_COPY
} SVGAFifoCmdFrontRopFill;


/*
 * SVGA_CMD_FENCE --
 *
 *    Insert a synchronization fence.  When the SVGA device reaches
 *    this command, it will copy the 'fence' value into the
 *    SVGA_FIFO_FENCE register. It will also compare the fence against
 *    SVGA_FIFO_FENCE_GOAL. If the fence matches the goal and the
 *    SVGA_IRQFLAG_FENCE_GOAL interrupt is enabled, the device will
 *    raise this interrupt.
 *
 * Availability:
 *    SVGA_FIFO_FENCE for this command,
 *    SVGA_CAP_IRQMASK for SVGA_FIFO_FENCE_GOAL.
 */

typedef
struct {
	uint32_t fence;
} SVGAFifoCmdFence;


/*
 * SVGA_CMD_ESCAPE --
 *
 *    Send an extended or vendor-specific variable length command.
 *    This is used for video overlay, third party plugins, and
 *    internal debugging tools. See svga_escape.h
 *
 * Availability:
 *    SVGA_FIFO_CAP_ESCAPE
 */

typedef
struct {
	uint32_t nsid;
	uint32_t size;
	/* followed by 'size' bytes of data */
} SVGAFifoCmdEscape;


/*
 * SVGA_CMD_DEFINE_SCREEN --
 *
 *    Define or redefine an SVGAScreenObject. See the description of
 *    SVGAScreenObject above.  The video driver is responsible for
 *    generating new screen IDs. They should be small positive
 *    integers. The virtual device will have an implementation
 *    specific upper limit on the number of screen IDs
 *    supported. Drivers are responsible for recycling IDs. The first
 *    valid ID is zero.
 *
 *    - Interaction with other registers:
 *
 *    For backwards compatibility, when the GFB mode registers (WIDTH,
 *    HEIGHT, PITCHLOCK, BITS_PER_PIXEL) are modified, the SVGA device
 *    deletes all screens other than screen #0, and redefines screen
 *    #0 according to the specified mode. Drivers that use
 *    SVGA_CMD_DEFINE_SCREEN should destroy or redefine screen #0.
 *
 *    If you use screen objects, do not use the legacy multi-mon
 *    registers (SVGA_REG_NUM_GUEST_DISPLAYS, SVGA_REG_DISPLAY_*).
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
 */

typedef
struct {
	SVGAScreenObject screen;   // Variable-length according to version
} SVGAFifoCmdDefineScreen;


/*
 * SVGA_CMD_DESTROY_SCREEN --
 *
 *    Destroy an SVGAScreenObject. Its ID is immediately available for
 *    re-use.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
 */

typedef
struct {
	uint32_t screenId;
} SVGAFifoCmdDestroyScreen;


/*
 * SVGA_CMD_DEFINE_GMRFB --
 *
 *    This command sets a piece of SVGA device state called the
 *    Guest Memory Region Framebuffer, or GMRFB. The GMRFB is a
 *    piece of light-weight state which identifies the location and
 *    format of an image in guest memory or in BAR1. The GMRFB has
 *    an arbitrary size, and it doesn't need to match the geometry
 *    of the GFB or any screen object.
 *
 *    The GMRFB can be redefined as often as you like. You could
 *    always use the same GMRFB, you could redefine it before
 *    rendering from a different guest screen, or you could even
 *    redefine it before every blit.
 *
 *    There are multiple ways to use this command. The simplest way is
 *    to use it to move the framebuffer either to elsewhere in the GFB
 *    (BAR1) memory region, or to a user-defined GMR. This lets a
 *    driver use a framebuffer allocated entirely out of normal system
 *    memory, which we encourage.
 *
 *    Another way to use this command is to set up a ring buffer of
 *    updates in GFB memory. If a driver wants to ensure that no
 *    frames are skipped by the SVGA device, it is important that the
 *    driver not modify the source data for a blit until the device is
 *    done processing the command. One efficient way to accomplish
 *    this is to use a ring of small DMA buffers. Each buffer is used
 *    for one blit, then we move on to the next buffer in the
 *    ring. The FENCE mechanism is used to protect each buffer from
 *    re-use until the device is finished with that buffer's
 *    corresponding blit.
 *
 *    This command does not affect the meaning of SVGA_CMD_UPDATE.
 *    UPDATEs always occur from the legacy GFB memory area. This
 *    command has no support for pseudocolor GMRFBs. Currently only
 *    true-color 15, 16, and 24-bit depths are supported. Future
 *    devices may expose capabilities for additional framebuffer
 *    formats.
 *
 *    The default GMRFB value is undefined. Drivers must always send
 *    this command at least once before performing any blit from the
 *    GMRFB.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
 */

typedef
struct {
	SVGAGuestPtr        ptr;
	uint32_t              bytesPerLine;
	SVGAGMRImageFormat  format;
} SVGAFifoCmdDefineGMRFB;


/*
 * SVGA_CMD_BLIT_GMRFB_TO_SCREEN --
 *
 *    This is a guest-to-host blit. It performs a DMA operation to
 *    copy a rectangular region of pixels from the current GMRFB to
 *    one or more Screen Objects.
 *
 *    The destination coordinate may be specified relative to a
 *    screen's origin (if a screen ID is specified) or relative to the
 *    virtual coordinate system's origin (if the screen ID is
 *    SVGA_ID_INVALID). The actual destination may span zero or more
 *    screens, in the case of a virtual destination rect or a rect
 *    which extends off the edge of the specified screen.
 *
 *    This command writes to the screen's "base layer": the underlying
 *    framebuffer which exists below any cursor or video overlays. No
 *    action is necessary to explicitly hide or update any overlays
 *    which exist on top of the updated region.
 *
 *    The SVGA device is guaranteed to finish reading from the GMRFB
 *    by the time any subsequent FENCE commands are reached.
 *
 *    This command consumes an annotation. See the
 *    SVGA_CMD_ANNOTATION_* commands for details.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
 */

typedef
struct {
	SVGASignedPoint  srcOrigin;
	SVGASignedRect   destRect;
	uint32_t           destScreenId;
} SVGAFifoCmdBlitGMRFBToScreen;


/*
 * SVGA_CMD_BLIT_SCREEN_TO_GMRFB --
 *
 *    This is a host-to-guest blit. It performs a DMA operation to
 *    copy a rectangular region of pixels from a single Screen Object
 *    back to the current GMRFB.
 *
 *    Usage note: This command should be used rarely. It will
 *    typically be inefficient, but it is necessary for some types of
 *    synchronization between 3D (GPU) and 2D (CPU) rendering into
 *    overlapping areas of a screen.
 *
 *    The source coordinate is specified relative to a screen's
 *    origin. The provided screen ID must be valid. If any parameters
 *    are invalid, the resulting pixel values are undefined.
 *
 *    This command reads the screen's "base layer". Overlays like
 *    video and cursor are not included, but any data which was sent
 *    using a blit-to-screen primitive will be available, no matter
 *    whether the data's original source was the GMRFB or the 3D
 *    acceleration hardware.
 *
 *    Note that our guest-to-host blits and host-to-guest blits aren't
 *    symmetric in their current implementation. While the parameters
 *    are identical, host-to-guest blits are a lot less featureful.
 *    They do not support clipping: If the source parameters don't
 *    fully fit within a screen, the blit fails. They must originate
 *    from exactly one screen. Virtual coordinates are not directly
 *    supported.
 *
 *    Host-to-guest blits do support the same set of GMRFB formats
 *    offered by guest-to-host blits.
 *
 *    The SVGA device is guaranteed to finish writing to the GMRFB by
 *    the time any subsequent FENCE commands are reached.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
 */

typedef
struct {
	SVGASignedPoint  destOrigin;
	SVGASignedRect   srcRect;
	uint32_t           srcScreenId;
} SVGAFifoCmdBlitScreenToGMRFB;


/*
 * SVGA_CMD_ANNOTATION_FILL --
 *
 *    This is a blit annotation. This command stores a small piece of
 *    device state which is consumed by the next blit-to-screen
 *    command. The state is only cleared by commands which are
 *    specifically documented as consuming an annotation. Other
 *    commands (such as ESCAPEs for debugging) may intervene between
 *    the annotation and its associated blit.
 *
 *    This annotation is a promise about the contents of the next
 *    blit: The video driver is guaranteeing that all pixels in that
 *    blit will have the same value, specified here as a color in
 *    SVGAColorBGRX format.
 *
 *    The SVGA device can still render the blit correctly even if it
 *    ignores this annotation, but the annotation may allow it to
 *    perform the blit more efficiently, for example by ignoring the
 *    source data and performing a fill in hardware.
 *
 *    This annotation is most important for performance when the
 *    user's display is being remoted over a network connection.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
 */

typedef
struct {
	SVGAColorBGRX  color;
} SVGAFifoCmdAnnotationFill;


/*
 * SVGA_CMD_ANNOTATION_COPY --
 *
 *    This is a blit annotation. See SVGA_CMD_ANNOTATION_FILL for more
 *    information about annotations.
 *
 *    This annotation is a promise about the contents of the next
 *    blit: The video driver is guaranteeing that all pixels in that
 *    blit will have the same value as those which already exist at an
 *    identically-sized region on the same or a different screen.
 *
 *    Note that the source pixels for the COPY in this annotation are
 *    sampled before applying the anqnotation's associated blit. They
 *    are allowed to overlap with the blit's destination pixels.
 *
 *    The copy source rectangle is specified the same way as the blit
 *    destination: it can be a rectangle which spans zero or more
 *    screens, specified relative to either a screen or to the virtual
 *    coordinate system's origin. If the source rectangle includes
 *    pixels which are not from exactly one screen, the results are
 *    undefined.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT or SVGA_FIFO_CAP_SCREEN_OBJECT_2
 */

typedef
struct {
	SVGASignedPoint  srcOrigin;
	uint32_t           srcScreenId;
} SVGAFifoCmdAnnotationCopy;


/*
 * SVGA_CMD_DEFINE_GMR2 --
 *
 *    Define guest memory region v2.  See the description of GMRs above.
 *
 * Availability:
 *    SVGA_CAP_GMR2
 */

typedef
struct {
	uint32_t gmrId;
	uint32_t numPages;
}
SVGAFifoCmdDefineGMR2;


/*
 * SVGA_CMD_REMAP_GMR2 --
 *
 *    Remap guest memory region v2.  See the description of GMRs above.
 *
 *    This command allows guest to modify a portion of an existing GMR by
 *    invalidating it or reassigning it to different guest physical pages.
 *    The pages are identified by physical page number (PPN).  The pages
 *    are assumed to be pinned and valid for DMA operations.
 *
 *    Description of command flags:
 *
 *    SVGA_REMAP_GMR2_VIA_GMR: If enabled, references a PPN list in a GMR.
 *       The PPN list must not overlap with the remap region (this can be
 *       handled trivially by referencing a separate GMR).  If flag is
 *       disabled, PPN list is appended to SVGARemapGMR command.
 *
 *    SVGA_REMAP_GMR2_PPN64: If set, PPN list is in PPN64 format, otherwise
 *       it is in PPN32 format.
 *
 *    SVGA_REMAP_GMR2_SINGLE_PPN: If set, PPN list contains a single entry.
 *       A single PPN can be used to invalidate a portion of a GMR or
 *       map it to to a single guest scratch page.
 *
 * Availability:
 *    SVGA_CAP_GMR2
 */

typedef enum {
	SVGA_REMAP_GMR2_PPN32         = 0,
	SVGA_REMAP_GMR2_VIA_GMR       = (1 << 0),
	SVGA_REMAP_GMR2_PPN64         = (1 << 1),
	SVGA_REMAP_GMR2_SINGLE_PPN    = (1 << 2),
} SVGARemapGMR2Flags;

typedef
struct {
	uint32_t gmrId;
	SVGARemapGMR2Flags flags;
	uint32_t offsetPages; // offset in pages to begin remap
	uint32_t numPages; // number of pages to remap
	/*
	 * Followed by additional data depending on SVGARemapGMR2Flags.
	 *
	 * If flag SVGA_REMAP_GMR2_VIA_GMR is set, single SVGAGuestPtr follows.
	 * Otherwise an array of page descriptors in PPN32 or PPN64 format
	 * (according to flag SVGA_REMAP_GMR2_PPN64) follows.  If flag
	 * SVGA_REMAP_GMR2_SINGLE_PPN is set, array contains a single entry.
	 */
}
SVGAFifoCmdRemapGMR2;

/*
 * 3D Hardware Version
 *
 *   The hardware version is stored in the SVGA_FIFO_3D_HWVERSION fifo
 *   register.   Is set by the host and read by the guest.  This lets
 *   us make new guest drivers which are backwards-compatible with old
 *   SVGA hardware revisions.  It does not let us support old guest
 *   drivers.  Good enough for now.
 *
 */

#define SVGA3D_MAKE_HWVERSION(major, minor)      (((major) << 16) | ((minor) & 0xFF))
#define SVGA3D_MAJOR_HWVERSION(version)          ((version) >> 16)
#define SVGA3D_MINOR_HWVERSION(version)          ((version) & 0xFF)

typedef enum {
	SVGA3D_HWVERSION_WS5_RC1   = SVGA3D_MAKE_HWVERSION(0, 1),
	SVGA3D_HWVERSION_WS5_RC2   = SVGA3D_MAKE_HWVERSION(0, 2),
	SVGA3D_HWVERSION_WS51_RC1  = SVGA3D_MAKE_HWVERSION(0, 3),
	SVGA3D_HWVERSION_WS6_B1    = SVGA3D_MAKE_HWVERSION(1, 1),
	SVGA3D_HWVERSION_FUSION_11 = SVGA3D_MAKE_HWVERSION(1, 4),
	SVGA3D_HWVERSION_WS65_B1   = SVGA3D_MAKE_HWVERSION(2, 0),
	SVGA3D_HWVERSION_WS8_B1    = SVGA3D_MAKE_HWVERSION(2, 1),
	SVGA3D_HWVERSION_CURRENT   = SVGA3D_HWVERSION_WS8_B1,
} SVGA3dHardwareVersion;

/*
 * Generic Types
 */

typedef uint32_t SVGA3dBool; /* 32-bit Bool definition */
#define SVGA3D_NUM_CLIPPLANES                   6
#define SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS  8
#define SVGA3D_MAX_CONTEXT_IDS                  256
#define SVGA3D_MAX_SURFACE_IDS                  (32 * 1024)

/*
 * Surface formats.
 *
 * If you modify this list, be sure to keep GLUtil.c in sync. It
 * includes the internal format definition of each surface in
 * GLUtil_ConvertSurfaceFormat, and it contains a table of
 * human-readable names in GLUtil_GetFormatName.
 */

typedef enum SVGA3dSurfaceFormat {
	SVGA3D_FORMAT_INVALID               = 0,
	
	SVGA3D_X8R8G8B8                     = 1,
	SVGA3D_A8R8G8B8                     = 2,
	
	SVGA3D_R5G6B5                       = 3,
	SVGA3D_X1R5G5B5                     = 4,
	SVGA3D_A1R5G5B5                     = 5,
	SVGA3D_A4R4G4B4                     = 6,
	
	SVGA3D_Z_D32                        = 7,
	SVGA3D_Z_D16                        = 8,
	SVGA3D_Z_D24S8                      = 9,
	SVGA3D_Z_D15S1                      = 10,
	
	SVGA3D_LUMINANCE8                   = 11,
	SVGA3D_LUMINANCE4_ALPHA4            = 12,
	SVGA3D_LUMINANCE16                  = 13,
	SVGA3D_LUMINANCE8_ALPHA8            = 14,
	
	SVGA3D_DXT1                         = 15,
	SVGA3D_DXT2                         = 16,
	SVGA3D_DXT3                         = 17,
	SVGA3D_DXT4                         = 18,
	SVGA3D_DXT5                         = 19,
	
	SVGA3D_BUMPU8V8                     = 20,
	SVGA3D_BUMPL6V5U5                   = 21,
	SVGA3D_BUMPX8L8V8U8                 = 22,
	SVGA3D_BUMPL8V8U8                   = 23,
	
	SVGA3D_ARGB_S10E5                   = 24,   /* 16-bit floating-point ARGB */
	SVGA3D_ARGB_S23E8                   = 25,   /* 32-bit floating-point ARGB */
	
	SVGA3D_A2R10G10B10                  = 26,
	
	/* signed formats */
	SVGA3D_V8U8                         = 27,
	SVGA3D_Q8W8V8U8                     = 28,
	SVGA3D_CxV8U8                       = 29,
	
	/* mixed formats */
	SVGA3D_X8L8V8U8                     = 30,
	SVGA3D_A2W10V10U10                  = 31,
	
	SVGA3D_ALPHA8                       = 32,
	
	/* Single- and dual-component floating point formats */
	SVGA3D_R_S10E5                      = 33,
	SVGA3D_R_S23E8                      = 34,
	SVGA3D_RG_S10E5                     = 35,
	SVGA3D_RG_S23E8                     = 36,
	
	/*
	 * Any surface can be used as a buffer object, but SVGA3D_BUFFER is
	 * the most efficient format to use when creating new surfaces
	 * expressly for index or vertex data.
	 */
	
	SVGA3D_BUFFER                       = 37,
	
	SVGA3D_Z_D24X8                      = 38,
	
	SVGA3D_V16U16                       = 39,
	
	SVGA3D_G16R16                       = 40,
	SVGA3D_A16B16G16R16                 = 41,
	
	/* Packed Video formats */
	SVGA3D_UYVY                         = 42,
	SVGA3D_YUY2                         = 43,
	
	/* Planar video formats */
	SVGA3D_NV12                         = 44,
	
	/* Video format with alpha */
	SVGA3D_AYUV                         = 45,
	
	SVGA3D_BC4_UNORM                    = 108,
	SVGA3D_BC5_UNORM                    = 111,
	
	/* Advanced D3D9 depth formats. */
	SVGA3D_Z_DF16                       = 118,
	SVGA3D_Z_DF24                       = 119,
	SVGA3D_Z_D24S8_INT                  = 120,
	
	SVGA3D_FORMAT_MAX
} SVGA3dSurfaceFormat;

typedef uint32_t SVGA3dColor; /* a, r, g, b */

/*
 * These match the D3DFORMAT_OP definitions used by Direct3D. We need
 * them so that we can query the host for what the supported surface
 * operations are (when we're using the D3D backend, in particular),
 * and so we can send those operations to the guest.
 */
typedef enum {
	SVGA3DFORMAT_OP_TEXTURE                               = 0x00000001,
	SVGA3DFORMAT_OP_VOLUMETEXTURE                         = 0x00000002,
	SVGA3DFORMAT_OP_CUBETEXTURE                           = 0x00000004,
	SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET                = 0x00000008,
	SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET              = 0x00000010,
	SVGA3DFORMAT_OP_ZSTENCIL                              = 0x00000040,
	SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH   = 0x00000080,
	
	/*
	 * This format can be used as a render target if the current display mode
	 * is the same depth if the alpha channel is ignored. e.g. if the device
	 * can render to A8R8G8B8 when the display mode is X8R8G8B8, then the
	 * format op list entry for A8R8G8B8 should have this cap.
	 */
	SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET  = 0x00000100,
	
	/*
	 * This format contains DirectDraw support (including Flip).  This flag
	 * should not to be set on alpha formats.
	 */
	SVGA3DFORMAT_OP_DISPLAYMODE                           = 0x00000400,
	
	/*
	 * The rasterizer can support some level of Direct3D support in this format
	 * and implies that the driver can create a Context in this mode (for some
	 * render target format).  When this flag is set, the SVGA3DFORMAT_OP_DISPLAYMODE
	 * flag must also be set.
	 */
	SVGA3DFORMAT_OP_3DACCELERATION                        = 0x00000800,
	
	/*
	 * This is set for a private format when the driver has put the bpp in
	 * the structure.
	 */
	SVGA3DFORMAT_OP_PIXELSIZE                             = 0x00001000,
	
	/*
	 * Indicates that this format can be converted to any RGB format for which
	 * SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB is specified
	 */
	SVGA3DFORMAT_OP_CONVERT_TO_ARGB                       = 0x00002000,
	
	/*
	 * Indicates that this format can be used to create offscreen plain surfaces.
	 */
	SVGA3DFORMAT_OP_OFFSCREENPLAIN                        = 0x00004000,
	
	/*
	 * Indicated that this format can be read as an SRGB texture (meaning that the
	 * sampler will linearize the looked up data)
	 */
	SVGA3DFORMAT_OP_SRGBREAD                              = 0x00008000,
	
	/*
	 * Indicates that this format can be used in the bumpmap instructions
	 */
	SVGA3DFORMAT_OP_BUMPMAP                               = 0x00010000,
	
	/*
	 * Indicates that this format can be sampled by the displacement map sampler
	 */
	SVGA3DFORMAT_OP_DMAP                                  = 0x00020000,
	
	/*
	 * Indicates that this format cannot be used with texture filtering
	 */
	SVGA3DFORMAT_OP_NOFILTER                              = 0x00040000,
	
	/*
	 * Indicates that format conversions are supported to this RGB format if
	 * SVGA3DFORMAT_OP_CONVERT_TO_ARGB is specified in the source format.
	 */
	SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB                    = 0x00080000,
	
	/*
	 * Indicated that this format can be written as an SRGB target (meaning that the
	 * pixel pipe will DE-linearize data on output to format)
	 */
	SVGA3DFORMAT_OP_SRGBWRITE                             = 0x00100000,
	
	/*
	 * Indicates that this format cannot be used with alpha blending
	 */
	SVGA3DFORMAT_OP_NOALPHABLEND                          = 0x00200000,
	
	/*
	 * Indicates that the device can auto-generated sublevels for resources
	 * of this format
	 */
	SVGA3DFORMAT_OP_AUTOGENMIPMAP                         = 0x00400000,
	
	/*
	 * Indicates that this format can be used by vertex texture sampler
	 */
	SVGA3DFORMAT_OP_VERTEXTEXTURE                         = 0x00800000,
	
	/*
	 * Indicates that this format supports neither texture coordinate wrap
	 * modes, nor mipmapping
	 */
	SVGA3DFORMAT_OP_NOTEXCOORDWRAPNORMIP                  = 0x01000000
} SVGA3dFormatOp;

/*
 * This structure is a conversion of SVGA3DFORMAT_OP_*.
 * Entries must be located at the same position.
 */
typedef union {
	uint32_t value;
	struct {
		uint32_t texture : 1;
		uint32_t volumeTexture : 1;
		uint32_t cubeTexture : 1;
		uint32_t offscreenRenderTarget : 1;
		uint32_t sameFormatRenderTarget : 1;
		uint32_t unknown1 : 1;
		uint32_t zStencil : 1;
		uint32_t zStencilArbitraryDepth : 1;
		uint32_t sameFormatUpToAlpha : 1;
		uint32_t unknown2 : 1;
		uint32_t displayMode : 1;
		uint32_t acceleration3d : 1;
		uint32_t pixelSize : 1;
		uint32_t convertToARGB : 1;
		uint32_t offscreenPlain : 1;
		uint32_t sRGBRead : 1;
		uint32_t bumpMap : 1;
		uint32_t dmap : 1;
		uint32_t noFilter : 1;
		uint32_t memberOfGroupARGB : 1;
		uint32_t sRGBWrite : 1;
		uint32_t noAlphaBlend : 1;
		uint32_t autoGenMipMap : 1;
		uint32_t vertexTexture : 1;
		uint32_t noTexCoordWrapNorMip : 1;
	};
} SVGA3dSurfaceFormatCaps;

/*
 * SVGA_3D_CMD_SETRENDERSTATE Types.  All value types
 * must fit in a uint32_t.
 */

typedef enum {
	SVGA3D_RS_INVALID                   = 0,
	SVGA3D_RS_ZENABLE                   = 1,     /* SVGA3dBool */
	SVGA3D_RS_ZWRITEENABLE              = 2,     /* SVGA3dBool */
	SVGA3D_RS_ALPHATESTENABLE           = 3,     /* SVGA3dBool */
	SVGA3D_RS_DITHERENABLE              = 4,     /* SVGA3dBool */
	SVGA3D_RS_BLENDENABLE               = 5,     /* SVGA3dBool */
	SVGA3D_RS_FOGENABLE                 = 6,     /* SVGA3dBool */
	SVGA3D_RS_SPECULARENABLE            = 7,     /* SVGA3dBool */
	SVGA3D_RS_STENCILENABLE             = 8,     /* SVGA3dBool */
	SVGA3D_RS_LIGHTINGENABLE            = 9,     /* SVGA3dBool */
	SVGA3D_RS_NORMALIZENORMALS          = 10,    /* SVGA3dBool */
	SVGA3D_RS_POINTSPRITEENABLE         = 11,    /* SVGA3dBool */
	SVGA3D_RS_POINTSCALEENABLE          = 12,    /* SVGA3dBool */
	SVGA3D_RS_STENCILREF                = 13,    /* uint32_t */
	SVGA3D_RS_STENCILMASK               = 14,    /* uint32_t */
	SVGA3D_RS_STENCILWRITEMASK          = 15,    /* uint32_t */
	SVGA3D_RS_FOGSTART                  = 16,    /* float */
	SVGA3D_RS_FOGEND                    = 17,    /* float */
	SVGA3D_RS_FOGDENSITY                = 18,    /* float */
	SVGA3D_RS_POINTSIZE                 = 19,    /* float */
	SVGA3D_RS_POINTSIZEMIN              = 20,    /* float */
	SVGA3D_RS_POINTSIZEMAX              = 21,    /* float */
	SVGA3D_RS_POINTSCALE_A              = 22,    /* float */
	SVGA3D_RS_POINTSCALE_B              = 23,    /* float */
	SVGA3D_RS_POINTSCALE_C              = 24,    /* float */
	SVGA3D_RS_FOGCOLOR                  = 25,    /* SVGA3dColor */
	SVGA3D_RS_AMBIENT                   = 26,    /* SVGA3dColor */
	SVGA3D_RS_CLIPPLANEENABLE           = 27,    /* SVGA3dClipPlanes */
	SVGA3D_RS_FOGMODE                   = 28,    /* SVGA3dFogMode */
	SVGA3D_RS_FILLMODE                  = 29,    /* SVGA3dFillMode */
	SVGA3D_RS_SHADEMODE                 = 30,    /* SVGA3dShadeMode */
	SVGA3D_RS_LINEPATTERN               = 31,    /* SVGA3dLinePattern */
	SVGA3D_RS_SRCBLEND                  = 32,    /* SVGA3dBlendOp */
	SVGA3D_RS_DSTBLEND                  = 33,    /* SVGA3dBlendOp */
	SVGA3D_RS_BLENDEQUATION             = 34,    /* SVGA3dBlendEquation */
	SVGA3D_RS_CULLMODE                  = 35,    /* SVGA3dFace */
	SVGA3D_RS_ZFUNC                     = 36,    /* SVGA3dCmpFunc */
	SVGA3D_RS_ALPHAFUNC                 = 37,    /* SVGA3dCmpFunc */
	SVGA3D_RS_STENCILFUNC               = 38,    /* SVGA3dCmpFunc */
	SVGA3D_RS_STENCILFAIL               = 39,    /* SVGA3dStencilOp */
	SVGA3D_RS_STENCILZFAIL              = 40,    /* SVGA3dStencilOp */
	SVGA3D_RS_STENCILPASS               = 41,    /* SVGA3dStencilOp */
	SVGA3D_RS_ALPHAREF                  = 42,    /* float (0.0 .. 1.0) */
	SVGA3D_RS_FRONTWINDING              = 43,    /* SVGA3dFrontWinding */
	SVGA3D_RS_COORDINATETYPE            = 44,    /* SVGA3dCoordinateType */
	SVGA3D_RS_ZBIAS                     = 45,    /* float */
	SVGA3D_RS_RANGEFOGENABLE            = 46,    /* SVGA3dBool */
	SVGA3D_RS_COLORWRITEENABLE          = 47,    /* SVGA3dColorMask */
	SVGA3D_RS_VERTEXMATERIALENABLE      = 48,    /* SVGA3dBool */
	SVGA3D_RS_DIFFUSEMATERIALSOURCE     = 49,    /* SVGA3dVertexMaterial */
	SVGA3D_RS_SPECULARMATERIALSOURCE    = 50,    /* SVGA3dVertexMaterial */
	SVGA3D_RS_AMBIENTMATERIALSOURCE     = 51,    /* SVGA3dVertexMaterial */
	SVGA3D_RS_EMISSIVEMATERIALSOURCE    = 52,    /* SVGA3dVertexMaterial */
	SVGA3D_RS_TEXTUREFACTOR             = 53,    /* SVGA3dColor */
	SVGA3D_RS_LOCALVIEWER               = 54,    /* SVGA3dBool */
	SVGA3D_RS_SCISSORTESTENABLE         = 55,    /* SVGA3dBool */
	SVGA3D_RS_BLENDCOLOR                = 56,    /* SVGA3dColor */
	SVGA3D_RS_STENCILENABLE2SIDED       = 57,    /* SVGA3dBool */
	SVGA3D_RS_CCWSTENCILFUNC            = 58,    /* SVGA3dCmpFunc */
	SVGA3D_RS_CCWSTENCILFAIL            = 59,    /* SVGA3dStencilOp */
	SVGA3D_RS_CCWSTENCILZFAIL           = 60,    /* SVGA3dStencilOp */
	SVGA3D_RS_CCWSTENCILPASS            = 61,    /* SVGA3dStencilOp */
	SVGA3D_RS_VERTEXBLEND               = 62,    /* SVGA3dVertexBlendFlags */
	SVGA3D_RS_SLOPESCALEDEPTHBIAS       = 63,    /* float */
	SVGA3D_RS_DEPTHBIAS                 = 64,    /* float */
	
	
	/*
	 * Output Gamma Level
	 *
	 * Output gamma effects the gamma curve of colors that are output from the
	 * rendering pipeline.  A value of 1.0 specifies a linear color space. If the
	 * value is <= 0.0, gamma correction is ignored and linear color space is
	 * used.
	 */
	
	SVGA3D_RS_OUTPUTGAMMA               = 65,    /* float */
	SVGA3D_RS_ZVISIBLE                  = 66,    /* SVGA3dBool */
	SVGA3D_RS_LASTPIXEL                 = 67,    /* SVGA3dBool */
	SVGA3D_RS_CLIPPING                  = 68,    /* SVGA3dBool */
	SVGA3D_RS_WRAP0                     = 69,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP1                     = 70,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP2                     = 71,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP3                     = 72,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP4                     = 73,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP5                     = 74,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP6                     = 75,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP7                     = 76,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP8                     = 77,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP9                     = 78,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP10                    = 79,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP11                    = 80,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP12                    = 81,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP13                    = 82,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP14                    = 83,    /* SVGA3dWrapFlags */
	SVGA3D_RS_WRAP15                    = 84,    /* SVGA3dWrapFlags */
	SVGA3D_RS_MULTISAMPLEANTIALIAS      = 85,    /* SVGA3dBool */
	SVGA3D_RS_MULTISAMPLEMASK           = 86,    /* uint32_t */
	SVGA3D_RS_INDEXEDVERTEXBLENDENABLE  = 87,    /* SVGA3dBool */
	SVGA3D_RS_TWEENFACTOR               = 88,    /* float */
	SVGA3D_RS_ANTIALIASEDLINEENABLE     = 89,    /* SVGA3dBool */
	SVGA3D_RS_COLORWRITEENABLE1         = 90,    /* SVGA3dColorMask */
	SVGA3D_RS_COLORWRITEENABLE2         = 91,    /* SVGA3dColorMask */
	SVGA3D_RS_COLORWRITEENABLE3         = 92,    /* SVGA3dColorMask */
	SVGA3D_RS_SEPARATEALPHABLENDENABLE  = 93,    /* SVGA3dBool */
	SVGA3D_RS_SRCBLENDALPHA             = 94,    /* SVGA3dBlendOp */
	SVGA3D_RS_DSTBLENDALPHA             = 95,    /* SVGA3dBlendOp */
	SVGA3D_RS_BLENDEQUATIONALPHA        = 96,    /* SVGA3dBlendEquation */
	SVGA3D_RS_TRANSPARENCYANTIALIAS     = 97,    /* SVGA3dTransparencyAntialiasType */
	SVGA3D_RS_LINEAA                    = 98,    /* SVGA3dBool */
	SVGA3D_RS_LINEWIDTH                 = 99,    /* float */
	SVGA3D_RS_MAX
} SVGA3dRenderStateName;

typedef enum {
	SVGA3D_TRANSPARENCYANTIALIAS_NORMAL            = 0,
	SVGA3D_TRANSPARENCYANTIALIAS_ALPHATOCOVERAGE   = 1,
	SVGA3D_TRANSPARENCYANTIALIAS_SUPERSAMPLE       = 2,
	SVGA3D_TRANSPARENCYANTIALIAS_MAX
} SVGA3dTransparencyAntialiasType;

typedef enum {
	SVGA3D_VERTEXMATERIAL_NONE     = 0,    /* Use the value in the current material */
	SVGA3D_VERTEXMATERIAL_DIFFUSE  = 1,    /* Use the value in the diffuse component */
	SVGA3D_VERTEXMATERIAL_SPECULAR = 2,    /* Use the value in the specular component */
} SVGA3dVertexMaterial;

typedef enum {
	SVGA3D_FILLMODE_INVALID = 0,
	SVGA3D_FILLMODE_POINT   = 1,
	SVGA3D_FILLMODE_LINE    = 2,
	SVGA3D_FILLMODE_FILL    = 3,
	SVGA3D_FILLMODE_MAX
} SVGA3dFillModeType;


typedef
union {
	struct {
		uint16_t   mode;       /* SVGA3dFillModeType */
		uint16_t   face;       /* SVGA3dFace */
	};
	uint32_t uintValue;
} SVGA3dFillMode;

typedef enum {
	SVGA3D_SHADEMODE_INVALID = 0,
	SVGA3D_SHADEMODE_FLAT    = 1,
	SVGA3D_SHADEMODE_SMOOTH  = 2,
	SVGA3D_SHADEMODE_PHONG   = 3,     /* Not supported */
	SVGA3D_SHADEMODE_MAX
} SVGA3dShadeMode;

typedef
union {
	struct {
		uint16_t repeat;
		uint16_t pattern;
	};
	uint32_t uintValue;
} SVGA3dLinePattern;

typedef enum {
	SVGA3D_BLENDOP_INVALID            = 0,
	SVGA3D_BLENDOP_ZERO               = 1,
	SVGA3D_BLENDOP_ONE                = 2,
	SVGA3D_BLENDOP_SRCCOLOR           = 3,
	SVGA3D_BLENDOP_INVSRCCOLOR        = 4,
	SVGA3D_BLENDOP_SRCALPHA           = 5,
	SVGA3D_BLENDOP_INVSRCALPHA        = 6,
	SVGA3D_BLENDOP_DESTALPHA          = 7,
	SVGA3D_BLENDOP_INVDESTALPHA       = 8,
	SVGA3D_BLENDOP_DESTCOLOR          = 9,
	SVGA3D_BLENDOP_INVDESTCOLOR       = 10,
	SVGA3D_BLENDOP_SRCALPHASAT        = 11,
	SVGA3D_BLENDOP_BLENDFACTOR        = 12,
	SVGA3D_BLENDOP_INVBLENDFACTOR     = 13,
	SVGA3D_BLENDOP_MAX
} SVGA3dBlendOp;

typedef enum {
	SVGA3D_BLENDEQ_INVALID            = 0,
	SVGA3D_BLENDEQ_ADD                = 1,
	SVGA3D_BLENDEQ_SUBTRACT           = 2,
	SVGA3D_BLENDEQ_REVSUBTRACT        = 3,
	SVGA3D_BLENDEQ_MINIMUM            = 4,
	SVGA3D_BLENDEQ_MAXIMUM            = 5,
	SVGA3D_BLENDEQ_MAX
} SVGA3dBlendEquation;

typedef enum {
	SVGA3D_FRONTWINDING_INVALID = 0,
	SVGA3D_FRONTWINDING_CW      = 1,
	SVGA3D_FRONTWINDING_CCW     = 2,
	SVGA3D_FRONTWINDING_MAX
} SVGA3dFrontWinding;

typedef enum {
	SVGA3D_FACE_INVALID  = 0,
	SVGA3D_FACE_NONE     = 1,
	SVGA3D_FACE_FRONT    = 2,
	SVGA3D_FACE_BACK     = 3,
	SVGA3D_FACE_FRONT_BACK = 4,
	SVGA3D_FACE_MAX
} SVGA3dFace;

/*
 * The order and the values should not be changed
 */

typedef enum {
	SVGA3D_CMP_INVALID              = 0,
	SVGA3D_CMP_NEVER                = 1,
	SVGA3D_CMP_LESS                 = 2,
	SVGA3D_CMP_EQUAL                = 3,
	SVGA3D_CMP_LESSEQUAL            = 4,
	SVGA3D_CMP_GREATER              = 5,
	SVGA3D_CMP_NOTEQUAL             = 6,
	SVGA3D_CMP_GREATEREQUAL         = 7,
	SVGA3D_CMP_ALWAYS               = 8,
	SVGA3D_CMP_MAX
} SVGA3dCmpFunc;

/*
 * SVGA3D_FOGFUNC_* specifies the fog equation, or PER_VERTEX which allows
 * the fog factor to be specified in the alpha component of the specular
 * (a.k.a. secondary) vertex color.
 */
typedef enum {
	SVGA3D_FOGFUNC_INVALID          = 0,
	SVGA3D_FOGFUNC_EXP              = 1,
	SVGA3D_FOGFUNC_EXP2             = 2,
	SVGA3D_FOGFUNC_LINEAR           = 3,
	SVGA3D_FOGFUNC_PER_VERTEX       = 4
} SVGA3dFogFunction;

/*
 * SVGA3D_FOGTYPE_* specifies if fog factors are computed on a per-vertex
 * or per-pixel basis.
 */
typedef enum {
	SVGA3D_FOGTYPE_INVALID          = 0,
	SVGA3D_FOGTYPE_VERTEX           = 1,
	SVGA3D_FOGTYPE_PIXEL            = 2,
	SVGA3D_FOGTYPE_MAX              = 3
} SVGA3dFogType;

/*
 * SVGA3D_FOGBASE_* selects depth or range-based fog. Depth-based fog is
 * computed using the eye Z value of each pixel (or vertex), whereas range-
 * based fog is computed using the actual distance (range) to the eye.
 */
typedef enum {
	SVGA3D_FOGBASE_INVALID          = 0,
	SVGA3D_FOGBASE_DEPTHBASED       = 1,
	SVGA3D_FOGBASE_RANGEBASED       = 2,
	SVGA3D_FOGBASE_MAX              = 3
} SVGA3dFogBase;

typedef enum {
	SVGA3D_STENCILOP_INVALID        = 0,
	SVGA3D_STENCILOP_KEEP           = 1,
	SVGA3D_STENCILOP_ZERO           = 2,
	SVGA3D_STENCILOP_REPLACE        = 3,
	SVGA3D_STENCILOP_INCRSAT        = 4,
	SVGA3D_STENCILOP_DECRSAT        = 5,
	SVGA3D_STENCILOP_INVERT         = 6,
	SVGA3D_STENCILOP_INCR           = 7,
	SVGA3D_STENCILOP_DECR           = 8,
	SVGA3D_STENCILOP_MAX
} SVGA3dStencilOp;

typedef enum {
	SVGA3D_CLIPPLANE_0              = (1 << 0),
	SVGA3D_CLIPPLANE_1              = (1 << 1),
	SVGA3D_CLIPPLANE_2              = (1 << 2),
	SVGA3D_CLIPPLANE_3              = (1 << 3),
	SVGA3D_CLIPPLANE_4              = (1 << 4),
	SVGA3D_CLIPPLANE_5              = (1 << 5),
} SVGA3dClipPlanes;

typedef enum {
	SVGA3D_CLEAR_COLOR              = 0x1,
	SVGA3D_CLEAR_DEPTH              = 0x2,
	SVGA3D_CLEAR_STENCIL            = 0x4
} SVGA3dClearFlag;

typedef struct {
	uint32_t color;
	float depth;
	uint32_t stencil;
} SVGA3dClearValue;

typedef enum {
	SVGA3D_RT_DEPTH                 = 0,
	SVGA3D_RT_STENCIL               = 1,
	SVGA3D_RT_COLOR0                = 2,
	SVGA3D_RT_COLOR1                = 3,
	SVGA3D_RT_COLOR2                = 4,
	SVGA3D_RT_COLOR3                = 5,
	SVGA3D_RT_COLOR4                = 6,
	SVGA3D_RT_COLOR5                = 7,
	SVGA3D_RT_COLOR6                = 8,
	SVGA3D_RT_COLOR7                = 9,
	SVGA3D_RT_MAX,
	SVGA3D_RT_INVALID               = ((uint32_t)-1),
} SVGA3dRenderTargetType;

#define SVGA3D_MAX_RT_COLOR (SVGA3D_RT_COLOR7 - SVGA3D_RT_COLOR0 + 1)

typedef
union {
	struct {
		uint32_t  red   : 1;
		uint32_t  green : 1;
		uint32_t  blue  : 1;
		uint32_t  alpha : 1;
	};
	uint32_t uintValue;
} SVGA3dColorMask;

typedef enum {
	SVGA3D_VBLEND_DISABLE            = 0,
	SVGA3D_VBLEND_1WEIGHT            = 1,
	SVGA3D_VBLEND_2WEIGHT            = 2,
	SVGA3D_VBLEND_3WEIGHT            = 3,
} SVGA3dVertexBlendFlags;

typedef enum {
	SVGA3D_WRAPCOORD_0   = 1 << 0,
	SVGA3D_WRAPCOORD_1   = 1 << 1,
	SVGA3D_WRAPCOORD_2   = 1 << 2,
	SVGA3D_WRAPCOORD_3   = 1 << 3,
	SVGA3D_WRAPCOORD_ALL = 0xF,
} SVGA3dWrapFlags;

/*
 * SVGA_3D_CMD_TEXTURESTATE Types.  All value types
 * must fit in a uint32_t.
 */

typedef enum {
	SVGA3D_TS_INVALID                    = 0,
	SVGA3D_TS_BIND_TEXTURE               = 1,    /* SVGA3dSurfaceId */
	SVGA3D_TS_COLOROP                    = 2,    /* SVGA3dTextureCombiner */
	SVGA3D_TS_COLORARG1                  = 3,    /* SVGA3dTextureArgData */
	SVGA3D_TS_COLORARG2                  = 4,    /* SVGA3dTextureArgData */
	SVGA3D_TS_ALPHAOP                    = 5,    /* SVGA3dTextureCombiner */
	SVGA3D_TS_ALPHAARG1                  = 6,    /* SVGA3dTextureArgData */
	SVGA3D_TS_ALPHAARG2                  = 7,    /* SVGA3dTextureArgData */
	SVGA3D_TS_ADDRESSU                   = 8,    /* SVGA3dTextureAddress */
	SVGA3D_TS_ADDRESSV                   = 9,    /* SVGA3dTextureAddress */
	SVGA3D_TS_MIPFILTER                  = 10,   /* SVGA3dTextureFilter */
	SVGA3D_TS_MAGFILTER                  = 11,   /* SVGA3dTextureFilter */
	SVGA3D_TS_MINFILTER                  = 12,   /* SVGA3dTextureFilter */
	SVGA3D_TS_BORDERCOLOR                = 13,   /* SVGA3dColor */
	SVGA3D_TS_TEXCOORDINDEX              = 14,   /* uint32_t */
	SVGA3D_TS_TEXTURETRANSFORMFLAGS      = 15,   /* SVGA3dTexTransformFlags */
	SVGA3D_TS_TEXCOORDGEN                = 16,   /* SVGA3dTextureCoordGen */
	SVGA3D_TS_BUMPENVMAT00               = 17,   /* float */
	SVGA3D_TS_BUMPENVMAT01               = 18,   /* float */
	SVGA3D_TS_BUMPENVMAT10               = 19,   /* float */
	SVGA3D_TS_BUMPENVMAT11               = 20,   /* float */
	SVGA3D_TS_TEXTURE_MIPMAP_LEVEL       = 21,   /* uint32_t */
	SVGA3D_TS_TEXTURE_LOD_BIAS           = 22,   /* float */
	SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL  = 23,   /* uint32_t */
	SVGA3D_TS_ADDRESSW                   = 24,   /* SVGA3dTextureAddress */
	
	
	/*
	 * Sampler Gamma Level
	 *
	 * Sampler gamma effects the color of samples taken from the sampler.  A
	 * value of 1.0 will produce linear samples.  If the value is <= 0.0 the
	 * gamma value is ignored and a linear space is used.
	 */
	
	SVGA3D_TS_GAMMA                      = 25,   /* float */
	SVGA3D_TS_BUMPENVLSCALE              = 26,   /* float */
	SVGA3D_TS_BUMPENVLOFFSET             = 27,   /* float */
	SVGA3D_TS_COLORARG0                  = 28,   /* SVGA3dTextureArgData */
	SVGA3D_TS_ALPHAARG0                  = 29,   /* SVGA3dTextureArgData */
	SVGA3D_TS_MAX
} SVGA3dTextureStateName;

typedef enum {
	SVGA3D_TC_INVALID                   = 0,
	SVGA3D_TC_DISABLE                   = 1,
	SVGA3D_TC_SELECTARG1                = 2,
	SVGA3D_TC_SELECTARG2                = 3,
	SVGA3D_TC_MODULATE                  = 4,
	SVGA3D_TC_ADD                       = 5,
	SVGA3D_TC_ADDSIGNED                 = 6,
	SVGA3D_TC_SUBTRACT                  = 7,
	SVGA3D_TC_BLENDTEXTUREALPHA         = 8,
	SVGA3D_TC_BLENDDIFFUSEALPHA         = 9,
	SVGA3D_TC_BLENDCURRENTALPHA         = 10,
	SVGA3D_TC_BLENDFACTORALPHA          = 11,
	SVGA3D_TC_MODULATE2X                = 12,
	SVGA3D_TC_MODULATE4X                = 13,
	SVGA3D_TC_DSDT                      = 14,
	SVGA3D_TC_DOTPRODUCT3               = 15,
	SVGA3D_TC_BLENDTEXTUREALPHAPM       = 16,
	SVGA3D_TC_ADDSIGNED2X               = 17,
	SVGA3D_TC_ADDSMOOTH                 = 18,
	SVGA3D_TC_PREMODULATE               = 19,
	SVGA3D_TC_MODULATEALPHA_ADDCOLOR    = 20,
	SVGA3D_TC_MODULATECOLOR_ADDALPHA    = 21,
	SVGA3D_TC_MODULATEINVALPHA_ADDCOLOR = 22,
	SVGA3D_TC_MODULATEINVCOLOR_ADDALPHA = 23,
	SVGA3D_TC_BUMPENVMAPLUMINANCE       = 24,
	SVGA3D_TC_MULTIPLYADD               = 25,
	SVGA3D_TC_LERP                      = 26,
	SVGA3D_TC_MAX
} SVGA3dTextureCombiner;

#define SVGA3D_TC_CAP_BIT(svga3d_tc_op) (svga3d_tc_op ? (1 << (svga3d_tc_op - 1)) : 0)

typedef enum {
	SVGA3D_TEX_ADDRESS_INVALID    = 0,
	SVGA3D_TEX_ADDRESS_WRAP       = 1,
	SVGA3D_TEX_ADDRESS_MIRROR     = 2,
	SVGA3D_TEX_ADDRESS_CLAMP      = 3,
	SVGA3D_TEX_ADDRESS_BORDER     = 4,
	SVGA3D_TEX_ADDRESS_MIRRORONCE = 5,
	SVGA3D_TEX_ADDRESS_EDGE       = 6,
	SVGA3D_TEX_ADDRESS_MAX
} SVGA3dTextureAddress;

/*
 * SVGA3D_TEX_FILTER_NONE as the minification filter means mipmapping is
 * disabled, and the rasterizer should use the magnification filter instead.
 */
typedef enum {
	SVGA3D_TEX_FILTER_NONE           = 0,
	SVGA3D_TEX_FILTER_NEAREST        = 1,
	SVGA3D_TEX_FILTER_LINEAR         = 2,
	SVGA3D_TEX_FILTER_ANISOTROPIC    = 3,
	SVGA3D_TEX_FILTER_FLATCUBIC      = 4, // Deprecated, not implemented
	SVGA3D_TEX_FILTER_GAUSSIANCUBIC  = 5, // Deprecated, not implemented
	SVGA3D_TEX_FILTER_PYRAMIDALQUAD  = 6, // Not currently implemented
	SVGA3D_TEX_FILTER_GAUSSIANQUAD   = 7, // Not currently implemented
	SVGA3D_TEX_FILTER_MAX
} SVGA3dTextureFilter;

typedef enum {
	SVGA3D_TEX_TRANSFORM_OFF    = 0,
	SVGA3D_TEX_TRANSFORM_S      = (1 << 0),
	SVGA3D_TEX_TRANSFORM_T      = (1 << 1),
	SVGA3D_TEX_TRANSFORM_R      = (1 << 2),
	SVGA3D_TEX_TRANSFORM_Q      = (1 << 3),
	SVGA3D_TEX_PROJECTED        = (1 << 15),
} SVGA3dTexTransformFlags;

typedef enum {
	SVGA3D_TEXCOORD_GEN_OFF              = 0,
	SVGA3D_TEXCOORD_GEN_EYE_POSITION     = 1,
	SVGA3D_TEXCOORD_GEN_EYE_NORMAL       = 2,
	SVGA3D_TEXCOORD_GEN_REFLECTIONVECTOR = 3,
	SVGA3D_TEXCOORD_GEN_SPHERE           = 4,
	SVGA3D_TEXCOORD_GEN_MAX
} SVGA3dTextureCoordGen;

/*
 * Texture argument constants for texture combiner
 */
typedef enum {
	SVGA3D_TA_INVALID    = 0,
	SVGA3D_TA_CONSTANT   = 1,
	SVGA3D_TA_PREVIOUS   = 2,
	SVGA3D_TA_DIFFUSE    = 3,
	SVGA3D_TA_TEXTURE    = 4,
	SVGA3D_TA_SPECULAR   = 5,
	SVGA3D_TA_MAX
} SVGA3dTextureArgData;

#define SVGA3D_TM_MASK_LEN 4

/* Modifiers for texture argument constants defined above. */
typedef enum {
	SVGA3D_TM_NONE       = 0,
	SVGA3D_TM_ALPHA      = (1 << SVGA3D_TM_MASK_LEN),
	SVGA3D_TM_ONE_MINUS  = (2 << SVGA3D_TM_MASK_LEN),
} SVGA3dTextureArgModifier;

#define SVGA3D_INVALID_ID         ((uint32_t)-1)
#define SVGA3D_MAX_CLIP_PLANES    6

/*
 * This is the limit to the number of fixed-function texture
 * transforms and texture coordinates we can support. It does *not*
 * correspond to the number of texture image units (samplers) we
 * support!
 */
#define SVGA3D_MAX_TEXTURE_COORDS 8

/*
 * Vertex declarations
 *
 * Notes:
 *
 * SVGA3D_DECLUSAGE_POSITIONT is for pre-transformed vertices. If you
 * draw with any POSITIONT vertex arrays, the programmable vertex
 * pipeline will be implicitly disabled. Drawing will take place as if
 * no vertex shader was bound.
 */

typedef enum {
	SVGA3D_DECLUSAGE_POSITION     = 0,
	SVGA3D_DECLUSAGE_BLENDWEIGHT,       //  1
	SVGA3D_DECLUSAGE_BLENDINDICES,      //  2
	SVGA3D_DECLUSAGE_NORMAL,            //  3
	SVGA3D_DECLUSAGE_PSIZE,             //  4
	SVGA3D_DECLUSAGE_TEXCOORD,          //  5
	SVGA3D_DECLUSAGE_TANGENT,           //  6
	SVGA3D_DECLUSAGE_BINORMAL,          //  7
	SVGA3D_DECLUSAGE_TESSFACTOR,        //  8
	SVGA3D_DECLUSAGE_POSITIONT,         //  9
	SVGA3D_DECLUSAGE_COLOR,             // 10
	SVGA3D_DECLUSAGE_FOG,               // 11
	SVGA3D_DECLUSAGE_DEPTH,             // 12
	SVGA3D_DECLUSAGE_SAMPLE,            // 13
	SVGA3D_DECLUSAGE_MAX
} SVGA3dDeclUsage;

typedef enum {
	SVGA3D_DECLMETHOD_DEFAULT     = 0,
	SVGA3D_DECLMETHOD_PARTIALU,
	SVGA3D_DECLMETHOD_PARTIALV,
	SVGA3D_DECLMETHOD_CROSSUV,          // Normal
	SVGA3D_DECLMETHOD_UV,
	SVGA3D_DECLMETHOD_LOOKUP,           // Lookup a displacement map
	SVGA3D_DECLMETHOD_LOOKUPPRESAMPLED, // Lookup a pre-sampled displacement map
} SVGA3dDeclMethod;

typedef enum {
	SVGA3D_DECLTYPE_FLOAT1        =  0,
	SVGA3D_DECLTYPE_FLOAT2        =  1,
	SVGA3D_DECLTYPE_FLOAT3        =  2,
	SVGA3D_DECLTYPE_FLOAT4        =  3,
	SVGA3D_DECLTYPE_D3DCOLOR      =  4,
	SVGA3D_DECLTYPE_UBYTE4        =  5,
	SVGA3D_DECLTYPE_SHORT2        =  6,
	SVGA3D_DECLTYPE_SHORT4        =  7,
	SVGA3D_DECLTYPE_UBYTE4N       =  8,
	SVGA3D_DECLTYPE_SHORT2N       =  9,
	SVGA3D_DECLTYPE_SHORT4N       = 10,
	SVGA3D_DECLTYPE_USHORT2N      = 11,
	SVGA3D_DECLTYPE_USHORT4N      = 12,
	SVGA3D_DECLTYPE_UDEC3         = 13,
	SVGA3D_DECLTYPE_DEC3N         = 14,
	SVGA3D_DECLTYPE_FLOAT16_2     = 15,
	SVGA3D_DECLTYPE_FLOAT16_4     = 16,
	SVGA3D_DECLTYPE_MAX,
} SVGA3dDeclType;

/*
 * This structure is used for the divisor for geometry instancing;
 * it's a direct translation of the Direct3D equivalent.
 */
typedef union {
	struct {
		/*
		 * For index data, this number represents the number of instances to draw.
		 * For instance data, this number represents the number of
		 * instances/vertex in this stream
		 */
		uint32_t count : 30;
		
		/*
		 * This is 1 if this is supposed to be the data that is repeated for
		 * every instance.
		 */
		uint32_t indexedData : 1;
		
		/*
		 * This is 1 if this is supposed to be the per-instance data.
		 */
		uint32_t instanceData : 1;
	};
	
	uint32_t value;
} SVGA3dVertexDivisor;

typedef enum {
	SVGA3D_PRIMITIVE_INVALID                     = 0,
	SVGA3D_PRIMITIVE_TRIANGLELIST                = 1,
	SVGA3D_PRIMITIVE_POINTLIST                   = 2,
	SVGA3D_PRIMITIVE_LINELIST                    = 3,
	SVGA3D_PRIMITIVE_LINESTRIP                   = 4,
	SVGA3D_PRIMITIVE_TRIANGLESTRIP               = 5,
	SVGA3D_PRIMITIVE_TRIANGLEFAN                 = 6,
	SVGA3D_PRIMITIVE_MAX
} SVGA3dPrimitiveType;

typedef enum {
	SVGA3D_COORDINATE_INVALID                   = 0,
	SVGA3D_COORDINATE_LEFTHANDED                = 1,
	SVGA3D_COORDINATE_RIGHTHANDED               = 2,
	SVGA3D_COORDINATE_MAX
} SVGA3dCoordinateType;

typedef enum {
	SVGA3D_TRANSFORM_INVALID                     = 0,
	SVGA3D_TRANSFORM_WORLD                       = 1,
	SVGA3D_TRANSFORM_VIEW                        = 2,
	SVGA3D_TRANSFORM_PROJECTION                  = 3,
	SVGA3D_TRANSFORM_TEXTURE0                    = 4,
	SVGA3D_TRANSFORM_TEXTURE1                    = 5,
	SVGA3D_TRANSFORM_TEXTURE2                    = 6,
	SVGA3D_TRANSFORM_TEXTURE3                    = 7,
	SVGA3D_TRANSFORM_TEXTURE4                    = 8,
	SVGA3D_TRANSFORM_TEXTURE5                    = 9,
	SVGA3D_TRANSFORM_TEXTURE6                    = 10,
	SVGA3D_TRANSFORM_TEXTURE7                    = 11,
	SVGA3D_TRANSFORM_WORLD1                      = 12,
	SVGA3D_TRANSFORM_WORLD2                      = 13,
	SVGA3D_TRANSFORM_WORLD3                      = 14,
	SVGA3D_TRANSFORM_MAX
} SVGA3dTransformType;

typedef enum {
	SVGA3D_LIGHTTYPE_INVALID                     = 0,
	SVGA3D_LIGHTTYPE_POINT                       = 1,
	SVGA3D_LIGHTTYPE_SPOT1                       = 2, /* 1-cone, in degrees */
	SVGA3D_LIGHTTYPE_SPOT2                       = 3, /* 2-cone, in radians */
	SVGA3D_LIGHTTYPE_DIRECTIONAL                 = 4,
	SVGA3D_LIGHTTYPE_MAX
} SVGA3dLightType;

typedef enum {
	SVGA3D_CUBEFACE_POSX                         = 0,
	SVGA3D_CUBEFACE_NEGX                         = 1,
	SVGA3D_CUBEFACE_POSY                         = 2,
	SVGA3D_CUBEFACE_NEGY                         = 3,
	SVGA3D_CUBEFACE_POSZ                         = 4,
	SVGA3D_CUBEFACE_NEGZ                         = 5,
} SVGA3dCubeFace;

typedef enum {
	SVGA3D_SHADERTYPE_VS                         = 1,
	SVGA3D_SHADERTYPE_PS                         = 2,
	SVGA3D_SHADERTYPE_MAX
} SVGA3dShaderType;

typedef enum {
	SVGA3D_CONST_TYPE_FLOAT                      = 0,
	SVGA3D_CONST_TYPE_INT                        = 1,
	SVGA3D_CONST_TYPE_BOOL                       = 2,
} SVGA3dShaderConstType;

#define SVGA3D_MAX_SURFACE_FACES                6

typedef enum {
	SVGA3D_STRETCH_BLT_POINT                     = 0,
	SVGA3D_STRETCH_BLT_LINEAR                    = 1,
	SVGA3D_STRETCH_BLT_MAX
} SVGA3dStretchBltMode;

typedef enum {
	SVGA3D_QUERYTYPE_OCCLUSION                   = 0,
	SVGA3D_QUERYTYPE_MAX
} SVGA3dQueryType;

typedef enum {
	SVGA3D_QUERYSTATE_PENDING     = 0,      /* Waiting on the host (set by guest) */
	SVGA3D_QUERYSTATE_SUCCEEDED   = 1,      /* Completed successfully (set by host) */
	SVGA3D_QUERYSTATE_FAILED      = 2,      /* Completed unsuccessfully (set by host) */
	SVGA3D_QUERYSTATE_NEW         = 3,      /* Never submitted (For guest use only) */
} SVGA3dQueryState;

typedef enum {
	SVGA3D_WRITE_HOST_VRAM        = 1,
	SVGA3D_READ_HOST_VRAM         = 2,
} SVGA3dTransferType;

/*
 * The maximum number of vertex arrays we're guaranteed to support in
 * SVGA_3D_CMD_DRAWPRIMITIVES.
 */
#define SVGA3D_MAX_VERTEX_ARRAYS   32

/*
 * The maximum number of primitive ranges we're guaranteed to support
 * in SVGA_3D_CMD_DRAWPRIMITIVES.
 */
#define SVGA3D_MAX_DRAW_PRIMITIVE_RANGES 32

/*
 * Identifiers for commands in the command FIFO.
 *
 * IDs between 1000 and 1039 (inclusive) were used by obsolete versions of
 * the SVGA3D protocol and remain reserved; they should not be used in the
 * future.
 *
 * IDs between 1040 and 1999 (inclusive) are available for use by the
 * current SVGA3D protocol.
 *
 * FIFO clients other than SVGA3D should stay below 1000, or at 2000
 * and up.
 */

#define SVGA_3D_CMD_LEGACY_BASE            1000
#define SVGA_3D_CMD_BASE                   1040

#define SVGA_3D_CMD_SURFACE_DEFINE         SVGA_3D_CMD_BASE + 0     // Deprecated
#define SVGA_3D_CMD_SURFACE_DESTROY        SVGA_3D_CMD_BASE + 1
#define SVGA_3D_CMD_SURFACE_COPY           SVGA_3D_CMD_BASE + 2
#define SVGA_3D_CMD_SURFACE_STRETCHBLT     SVGA_3D_CMD_BASE + 3
#define SVGA_3D_CMD_SURFACE_DMA            SVGA_3D_CMD_BASE + 4
#define SVGA_3D_CMD_CONTEXT_DEFINE         SVGA_3D_CMD_BASE + 5
#define SVGA_3D_CMD_CONTEXT_DESTROY        SVGA_3D_CMD_BASE + 6
#define SVGA_3D_CMD_SETTRANSFORM           SVGA_3D_CMD_BASE + 7
#define SVGA_3D_CMD_SETZRANGE              SVGA_3D_CMD_BASE + 8
#define SVGA_3D_CMD_SETRENDERSTATE         SVGA_3D_CMD_BASE + 9
#define SVGA_3D_CMD_SETRENDERTARGET        SVGA_3D_CMD_BASE + 10
#define SVGA_3D_CMD_SETTEXTURESTATE        SVGA_3D_CMD_BASE + 11
#define SVGA_3D_CMD_SETMATERIAL            SVGA_3D_CMD_BASE + 12
#define SVGA_3D_CMD_SETLIGHTDATA           SVGA_3D_CMD_BASE + 13
#define SVGA_3D_CMD_SETLIGHTENABLED        SVGA_3D_CMD_BASE + 14
#define SVGA_3D_CMD_SETVIEWPORT            SVGA_3D_CMD_BASE + 15
#define SVGA_3D_CMD_SETCLIPPLANE           SVGA_3D_CMD_BASE + 16
#define SVGA_3D_CMD_CLEAR                  SVGA_3D_CMD_BASE + 17
#define SVGA_3D_CMD_PRESENT                SVGA_3D_CMD_BASE + 18    // Deprecated
#define SVGA_3D_CMD_SHADER_DEFINE          SVGA_3D_CMD_BASE + 19
#define SVGA_3D_CMD_SHADER_DESTROY         SVGA_3D_CMD_BASE + 20
#define SVGA_3D_CMD_SET_SHADER             SVGA_3D_CMD_BASE + 21
#define SVGA_3D_CMD_SET_SHADER_CONST       SVGA_3D_CMD_BASE + 22
#define SVGA_3D_CMD_DRAW_PRIMITIVES        SVGA_3D_CMD_BASE + 23
#define SVGA_3D_CMD_SETSCISSORRECT         SVGA_3D_CMD_BASE + 24
#define SVGA_3D_CMD_BEGIN_QUERY            SVGA_3D_CMD_BASE + 25
#define SVGA_3D_CMD_END_QUERY              SVGA_3D_CMD_BASE + 26
#define SVGA_3D_CMD_WAIT_FOR_QUERY         SVGA_3D_CMD_BASE + 27
#define SVGA_3D_CMD_PRESENT_READBACK       SVGA_3D_CMD_BASE + 28    // Deprecated
#define SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN SVGA_3D_CMD_BASE + 29
#define SVGA_3D_CMD_SURFACE_DEFINE_V2      SVGA_3D_CMD_BASE + 30
#define SVGA_3D_CMD_GENERATE_MIPMAPS       SVGA_3D_CMD_BASE + 31
#define SVGA_3D_CMD_ACTIVATE_SURFACE       SVGA_3D_CMD_BASE + 40
#define SVGA_3D_CMD_DEACTIVATE_SURFACE     SVGA_3D_CMD_BASE + 41
#define SVGA_3D_CMD_MAX                    SVGA_3D_CMD_BASE + 42

#define SVGA_3D_CMD_FUTURE_MAX             2000

/*
 * Common substructures used in multiple FIFO commands:
 */

typedef struct {
	union {
		struct {
			uint16_t  function;       // SVGA3dFogFunction
			uint8_t   type;           // SVGA3dFogType
			uint8_t   base;           // SVGA3dFogBase
		};
		uint32_t     uintValue;
	};
} SVGA3dFogMode;

/*
 * Uniquely identify one image (a 1D/2D/3D array) from a surface. This
 * is a surface ID as well as face/mipmap indices.
 */

typedef
struct SVGA3dSurfaceImageId {
	uint32_t               sid;
	uint32_t               face;
	uint32_t               mipmap;
} SVGA3dSurfaceImageId;

typedef
struct SVGA3dGuestImage {
	SVGAGuestPtr         ptr;
	
	/*
	 * A note on interpretation of pitch: This value of pitch is the
	 * number of bytes between vertically adjacent image
	 * blocks. Normally this is the number of bytes between the first
	 * pixel of two adjacent scanlines. With compressed textures,
	 * however, this may represent the number of bytes between
	 * compression blocks rather than between rows of pixels.
	 *
	 * XXX: Compressed textures currently must be tightly packed in guest memory.
	 *
	 * If the image is 1-dimensional, pitch is ignored.
	 *
	 * If 'pitch' is zero, the SVGA3D device calculates a pitch value
	 * assuming each row of blocks is tightly packed.
	 */
	uint32_t pitch;
} SVGA3dGuestImage;


/*
 * FIFO command format definitions:
 */

/*
 * The data size header following cmdNum for every 3d command
 */
typedef
struct {
	uint32_t               id;
	uint32_t               size;
} SVGA3dCmdHeader;

/*
 * A surface is a hierarchy of host VRAM surfaces: 1D, 2D, or 3D, with
 * optional mipmaps and cube faces.
 */

typedef
struct {
	uint32_t               width;
	uint32_t               height;
	uint32_t               depth;
} SVGA3dSize;

typedef enum {
	SVGA3D_SURFACE_CUBEMAP              = (1 << 0),
	SVGA3D_SURFACE_HINT_STATIC          = (1 << 1),
	SVGA3D_SURFACE_HINT_DYNAMIC         = (1 << 2),
	SVGA3D_SURFACE_HINT_INDEXBUFFER     = (1 << 3),
	SVGA3D_SURFACE_HINT_VERTEXBUFFER    = (1 << 4),
	SVGA3D_SURFACE_HINT_TEXTURE         = (1 << 5),
	SVGA3D_SURFACE_HINT_RENDERTARGET    = (1 << 6),
	SVGA3D_SURFACE_HINT_DEPTHSTENCIL    = (1 << 7),
	SVGA3D_SURFACE_HINT_WRITEONLY       = (1 << 8),
	SVGA3D_SURFACE_MASKABLE_ANTIALIAS   = (1 << 9),
	SVGA3D_SURFACE_AUTOGENMIPMAPS       = (1 << 10),
} SVGA3dSurfaceFlags;

typedef
struct {
	uint32_t               numMipLevels;
} SVGA3dSurfaceFace;

typedef
struct {
	uint32_t                      sid;
	SVGA3dSurfaceFlags          surfaceFlags;
	SVGA3dSurfaceFormat         format;
	/*
	 * If surfaceFlags has SVGA3D_SURFACE_CUBEMAP bit set, all SVGA3dSurfaceFace
	 * structures must have the same value of numMipLevels field.
	 * Otherwise, all but the first SVGA3dSurfaceFace structures must have the
	 * numMipLevels set to 0.
	 */
	SVGA3dSurfaceFace           face[SVGA3D_MAX_SURFACE_FACES];
	/*
	 * Followed by an SVGA3dSize structure for each mip level in each face.
	 *
	 * A note on surface sizes: Sizes are always specified in pixels,
	 * even if the true surface size is not a multiple of the minimum
	 * block size of the surface's format. For example, a 3x3x1 DXT1
	 * compressed texture would actually be stored as a 4x4x1 image in
	 * memory.
	 */
} SVGA3dCmdDefineSurface;       /* SVGA_3D_CMD_SURFACE_DEFINE */

typedef
struct {
	uint32_t                      sid;
	SVGA3dSurfaceFlags          surfaceFlags;
	SVGA3dSurfaceFormat         format;
	/*
	 * If surfaceFlags has SVGA3D_SURFACE_CUBEMAP bit set, all SVGA3dSurfaceFace
	 * structures must have the same value of numMipLevels field.
	 * Otherwise, all but the first SVGA3dSurfaceFace structures must have the
	 * numMipLevels set to 0.
	 */
	SVGA3dSurfaceFace           face[SVGA3D_MAX_SURFACE_FACES];
	uint32_t                      multisampleCount;
	SVGA3dTextureFilter         autogenFilter;
	/*
	 * Followed by an SVGA3dSize structure for each mip level in each face.
	 *
	 * A note on surface sizes: Sizes are always specified in pixels,
	 * even if the true surface size is not a multiple of the minimum
	 * block size of the surface's format. For example, a 3x3x1 DXT1
	 * compressed texture would actually be stored as a 4x4x1 image in
	 * memory.
	 */
} SVGA3dCmdDefineSurface_v2;     /* SVGA_3D_CMD_SURFACE_DEFINE_V2 */

typedef
struct {
	uint32_t               sid;
} SVGA3dCmdDestroySurface;      /* SVGA_3D_CMD_SURFACE_DESTROY */

typedef
struct {
	uint32_t               cid;
} SVGA3dCmdDefineContext;       /* SVGA_3D_CMD_CONTEXT_DEFINE */

typedef
struct {
	uint32_t               cid;
} SVGA3dCmdDestroyContext;      /* SVGA_3D_CMD_CONTEXT_DESTROY */

typedef
struct {
	uint32_t               cid;
	SVGA3dClearFlag      clearFlag;
	uint32_t               color;
	float                depth;
	uint32_t               stencil;
	/* Followed by variable number of SVGA3dRect structures */
} SVGA3dCmdClear;               /* SVGA_3D_CMD_CLEAR */

typedef
struct SVGA3dCopyRect {
	uint32_t               x;
	uint32_t               y;
	uint32_t               w;
	uint32_t               h;
	uint32_t               srcx;
	uint32_t               srcy;
} SVGA3dCopyRect;

typedef
struct SVGA3dCopyBox {
	uint32_t               x;
	uint32_t               y;
	uint32_t               z;
	uint32_t               w;
	uint32_t               h;
	uint32_t               d;
	uint32_t               srcx;
	uint32_t               srcy;
	uint32_t               srcz;
} SVGA3dCopyBox;

typedef
struct {
	uint32_t               x;
	uint32_t               y;
	uint32_t               w;
	uint32_t               h;
} SVGA3dRect;

typedef
struct {
	uint32_t               x;
	uint32_t               y;
	uint32_t               z;
	uint32_t               w;
	uint32_t               h;
	uint32_t               d;
} SVGA3dBox;

typedef
struct {
	uint32_t               x;
	uint32_t               y;
	uint32_t               z;
} SVGA3dPoint;

typedef
struct {
	SVGA3dLightType      type;
	SVGA3dBool           inWorldSpace;
	float                diffuse[4];
	float                specular[4];
	float                ambient[4];
	float                position[4];
	float                direction[4];
	float                range;
	float                falloff;
	float                attenuation0;
	float                attenuation1;
	float                attenuation2;
	float                theta;
	float                phi;
} SVGA3dLightData;

typedef
struct {
	uint32_t               sid;
	/* Followed by variable number of SVGA3dCopyRect structures */
} SVGA3dCmdPresent;             /* SVGA_3D_CMD_PRESENT */

typedef
struct {
	SVGA3dRenderStateName   state;
	union {
		uint32_t               uintValue;
		float                floatValue;
	};
} SVGA3dRenderState;

typedef
struct {
	uint32_t               cid;
	/* Followed by variable number of SVGA3dRenderState structures */
} SVGA3dCmdSetRenderState;      /* SVGA_3D_CMD_SETRENDERSTATE */

typedef
struct {
	uint32_t                 cid;
	SVGA3dRenderTargetType type;
	SVGA3dSurfaceImageId   target;
} SVGA3dCmdSetRenderTarget;     /* SVGA_3D_CMD_SETRENDERTARGET */

typedef
struct {
	SVGA3dSurfaceImageId  src;
	SVGA3dSurfaceImageId  dest;
	/* Followed by variable number of SVGA3dCopyBox structures */
} SVGA3dCmdSurfaceCopy;               /* SVGA_3D_CMD_SURFACE_COPY */

typedef
struct {
	SVGA3dSurfaceImageId  src;
	SVGA3dSurfaceImageId  dest;
	SVGA3dBox             boxSrc;
	SVGA3dBox             boxDest;
	SVGA3dStretchBltMode  mode;
} SVGA3dCmdSurfaceStretchBlt;         /* SVGA_3D_CMD_SURFACE_STRETCHBLT */

typedef
struct {
	/*
	 * If the discard flag is present in a surface DMA operation, the host may
	 * discard the contents of the current mipmap level and face of the target
	 * surface before applying the surface DMA contents.
	 */
	uint32_t discard : 1;
	
	/*
	 * If the unsynchronized flag is present, the host may perform this upload
	 * without syncing to pending reads on this surface.
	 */
	uint32_t unsynchronized : 1;
	
	/*
	 * Guests *MUST* set the reserved bits to 0 before submitting the command
	 * suffix as future flags may occupy these bits.
	 */
	uint32_t reserved : 30;
} SVGA3dSurfaceDMAFlags;

typedef
struct {
	SVGA3dGuestImage      guest;
	SVGA3dSurfaceImageId  host;
	SVGA3dTransferType    transfer;
	/*
	 * Followed by variable number of SVGA3dCopyBox structures. For consistency
	 * in all clipping logic and coordinate translation, we define the
	 * "source" in each copyBox as the guest image and the
	 * "destination" as the host image, regardless of transfer
	 * direction.
	 *
	 * For efficiency, the SVGA3D device is free to copy more data than
	 * specified. For example, it may round copy boxes outwards such
	 * that they lie on particular alignment boundaries.
	 */
} SVGA3dCmdSurfaceDMA;                /* SVGA_3D_CMD_SURFACE_DMA */

/*
 * SVGA3dCmdSurfaceDMASuffix --
 *
 *    This is a command suffix that will appear after a SurfaceDMA command in
 *    the FIFO.  It contains some extra information that hosts may use to
 *    optimize performance or protect the guest.  This suffix exists to preserve
 *    backwards compatibility while also allowing for new functionality to be
 *    implemented.
 */

typedef
struct {
	uint32_t suffixSize;
	
	/*
	 * The maximum offset is used to determine the maximum offset from the
	 * guestPtr base address that will be accessed or written to during this
	 * surfaceDMA.  If the suffix is supported, the host will respect this
	 * boundary while performing surface DMAs.
	 *
	 * Defaults to MAX_uint32_t
	 */
	uint32_t maximumOffset;
	
	/*
	 * A set of flags that describes optimizations that the host may perform
	 * while performing this surface DMA operation.  The guest should never rely
	 * on behaviour that is different when these flags are set for correctness.
	 *
	 * Defaults to 0
	 */
	SVGA3dSurfaceDMAFlags flags;
} SVGA3dCmdSurfaceDMASuffix;

/*
 * SVGA_3D_CMD_DRAW_PRIMITIVES --
 *
 *   This command is the SVGA3D device's generic drawing entry point.
 *   It can draw multiple ranges of primitives, optionally using an
 *   index buffer, using an arbitrary collection of vertex buffers.
 *
 *   Each SVGA3dVertexDecl defines a distinct vertex array to bind
 *   during this draw call. The declarations specify which surface
 *   the vertex data lives in, what that vertex data is used for,
 *   and how to interpret it.
 *
 *   Each SVGA3dPrimitiveRange defines a collection of primitives
 *   to render using the same vertex arrays. An index buffer is
 *   optional.
 */

typedef
struct {
	/*
	 * A range hint is an optional specification for the range of indices
	 * in an SVGA3dArray that will be used. If 'last' is zero, it is assumed
	 * that the entire array will be used.
	 *
	 * These are only hints. The SVGA3D device may use them for
	 * performance optimization if possible, but it's also allowed to
	 * ignore these values.
	 */
	uint32_t               first;
	uint32_t               last;
} SVGA3dArrayRangeHint;

typedef
struct {
	/*
	 * Define the origin and shape of a vertex or index array. Both
	 * 'offset' and 'stride' are in bytes. The provided surface will be
	 * reinterpreted as a flat array of bytes in the same format used
	 * by surface DMA operations. To avoid unnecessary conversions, the
	 * surface should be created with the SVGA3D_BUFFER format.
	 *
	 * Index 0 in the array starts 'offset' bytes into the surface.
	 * Index 1 begins at byte 'offset + stride', etc. Array indices may
	 * not be negative.
	 */
	uint32_t               surfaceId;
	uint32_t               offset;
	uint32_t               stride;
} SVGA3dArray;

typedef
struct {
	/*
	 * Describe a vertex array's data type, and define how it is to be
	 * used by the fixed function pipeline or the vertex shader. It
	 * isn't useful to have two VertexDecls with the same
	 * VertexArrayIdentity in one draw call.
	 */
	SVGA3dDeclType       type;
	SVGA3dDeclMethod     method;
	SVGA3dDeclUsage      usage;
	uint32_t               usageIndex;
} SVGA3dVertexArrayIdentity;

typedef
struct {
	SVGA3dVertexArrayIdentity  identity;
	SVGA3dArray                array;
	SVGA3dArrayRangeHint       rangeHint;
} SVGA3dVertexDecl;

typedef
struct {
	/*
	 * Define a group of primitives to render, from sequential indices.
	 *
	 * The value of 'primitiveType' and 'primitiveCount' imply the
	 * total number of vertices that will be rendered.
	 */
	SVGA3dPrimitiveType  primType;
	uint32_t               primitiveCount;
	
	/*
	 * Optional index buffer. If indexArray.surfaceId is
	 * SVGA3D_INVALID_ID, we render without an index buffer. Rendering
	 * without an index buffer is identical to rendering with an index
	 * buffer containing the sequence [0, 1, 2, 3, ...].
	 *
	 * If an index buffer is in use, indexWidth specifies the width in
	 * bytes of each index value. It must be less than or equal to
	 * indexArray.stride.
	 *
	 * (Currently, the SVGA3D device requires index buffers to be tightly
	 * packed. In other words, indexWidth == indexArray.stride)
	 */
	SVGA3dArray          indexArray;
	uint32_t               indexWidth;
	
	/*
	 * Optional index bias. This number is added to all indices from
	 * indexArray before they are used as vertex array indices. This
	 * can be used in multiple ways:
	 *
	 *  - When not using an indexArray, this bias can be used to
	 *    specify where in the vertex arrays to begin rendering.
	 *
	 *  - A positive number here is equivalent to increasing the
	 *    offset in each vertex array.
	 *
	 *  - A negative number can be used to render using a small
	 *    vertex array and an index buffer that contains large
	 *    values. This may be used by some applications that
	 *    crop a vertex buffer without modifying their index
	 *    buffer.
	 *
	 * Note that rendering with a negative bias value may be slower and
	 * use more memory than rendering with a positive or zero bias.
	 */
	int32_t                indexBias;
} SVGA3dPrimitiveRange;

typedef
struct {
	uint32_t               cid;
	uint32_t               numVertexDecls;
	uint32_t               numRanges;
	
	/*
	 * There are two variable size arrays after the
	 * SVGA3dCmdDrawPrimitives structure. In order,
	 * they are:
	 *
	 * 1. SVGA3dVertexDecl, quantity 'numVertexDecls', but no more than
	 *    SVGA3D_MAX_VERTEX_ARRAYS;
	 * 2. SVGA3dPrimitiveRange, quantity 'numRanges', but no more than
	 *    SVGA3D_MAX_DRAW_PRIMITIVE_RANGES;
	 * 3. Optionally, SVGA3dVertexDivisor, quantity 'numVertexDecls' (contains
	 *    the frequency divisor for the corresponding vertex decl).
	 */
} SVGA3dCmdDrawPrimitives;      /* SVGA_3D_CMD_DRAWPRIMITIVES */

typedef
struct {
	uint32_t                   stage;
	SVGA3dTextureStateName   name;
	union {
		uint32_t                value;
		float                 floatValue;
	};
} SVGA3dTextureState;

typedef
struct {
	uint32_t               cid;
	/* Followed by variable number of SVGA3dTextureState structures */
} SVGA3dCmdSetTextureState;      /* SVGA_3D_CMD_SETTEXTURESTATE */

typedef
struct {
	uint32_t                   cid;
	SVGA3dTransformType      type;
	float                    matrix[16];
} SVGA3dCmdSetTransform;          /* SVGA_3D_CMD_SETTRANSFORM */

typedef
struct {
	float                min;
	float                max;
} SVGA3dZRange;

typedef
struct {
	uint32_t               cid;
	SVGA3dZRange         zRange;
} SVGA3dCmdSetZRange;             /* SVGA_3D_CMD_SETZRANGE */

typedef
struct {
	float                diffuse[4];
	float                ambient[4];
	float                specular[4];
	float                emissive[4];
	float                shininess;
} SVGA3dMaterial;

typedef
struct {
	uint32_t               cid;
	SVGA3dFace           face;
	SVGA3dMaterial       material;
} SVGA3dCmdSetMaterial;           /* SVGA_3D_CMD_SETMATERIAL */

typedef
struct {
	uint32_t               cid;
	uint32_t               index;
	SVGA3dLightData      data;
} SVGA3dCmdSetLightData;           /* SVGA_3D_CMD_SETLIGHTDATA */

typedef
struct {
	uint32_t               cid;
	uint32_t               index;
	uint32_t               enabled;
} SVGA3dCmdSetLightEnabled;      /* SVGA_3D_CMD_SETLIGHTENABLED */

typedef
struct {
	uint32_t               cid;
	SVGA3dRect           rect;
} SVGA3dCmdSetViewport;           /* SVGA_3D_CMD_SETVIEWPORT */

typedef
struct {
	uint32_t               cid;
	SVGA3dRect           rect;
} SVGA3dCmdSetScissorRect;         /* SVGA_3D_CMD_SETSCISSORRECT */

typedef
struct {
	uint32_t               cid;
	uint32_t               index;
	float                plane[4];
} SVGA3dCmdSetClipPlane;           /* SVGA_3D_CMD_SETCLIPPLANE */

typedef
struct {
	uint32_t               cid;
	uint32_t               shid;
	SVGA3dShaderType     type;
	/* Followed by variable number of DWORDs for shader bycode */
} SVGA3dCmdDefineShader;           /* SVGA_3D_CMD_SHADER_DEFINE */

typedef
struct {
	uint32_t               cid;
	uint32_t               shid;
	SVGA3dShaderType     type;
} SVGA3dCmdDestroyShader;         /* SVGA_3D_CMD_SHADER_DESTROY */

typedef
struct {
	uint32_t                  cid;
	uint32_t                  reg;     /* register number */
	SVGA3dShaderType        type;
	SVGA3dShaderConstType   ctype;
	uint32_t                  values[4];
} SVGA3dCmdSetShaderConst;        /* SVGA_3D_CMD_SET_SHADER_CONST */

typedef
struct {
	uint32_t               cid;
	SVGA3dShaderType     type;
	uint32_t               shid;
} SVGA3dCmdSetShader;             /* SVGA_3D_CMD_SET_SHADER */

typedef
struct {
	uint32_t               cid;
	SVGA3dQueryType      type;
} SVGA3dCmdBeginQuery;           /* SVGA_3D_CMD_BEGIN_QUERY */

typedef
struct {
	uint32_t               cid;
	SVGA3dQueryType      type;
	SVGAGuestPtr         guestResult;  /* Points to an SVGA3dQueryResult structure */
} SVGA3dCmdEndQuery;                  /* SVGA_3D_CMD_END_QUERY */

typedef
struct {
	uint32_t               cid;          /* Same parameters passed to END_QUERY */
	SVGA3dQueryType      type;
	SVGAGuestPtr         guestResult;
} SVGA3dCmdWaitForQuery;              /* SVGA_3D_CMD_WAIT_FOR_QUERY */

typedef
struct {
	uint32_t               totalSize;    /* Set by guest before query is ended. */
	SVGA3dQueryState     state;        /* Set by host or guest. See SVGA3dQueryState. */
	union {                            /* Set by host on exit from PENDING state */
		uint32_t            result32;
	};
} SVGA3dQueryResult;

/*
 * SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN --
 *
 *    This is a blit from an SVGA3D surface to a Screen Object. Just
 *    like GMR-to-screen blits, this blit may be directed at a
 *    specific screen or to the virtual coordinate space.
 *
 *    The blit copies from a rectangular region of an SVGA3D surface
 *    image to a rectangular region of a screen or screens.
 *
 *    This command takes an optional variable-length list of clipping
 *    rectangles after the body of the command. If no rectangles are
 *    specified, there is no clipping region. The entire destRect is
 *    drawn to. If one or more rectangles are included, they describe
 *    a clipping region. The clip rectangle coordinates are measured
 *    relative to the top-left corner of destRect.
 *
 *    This clipping region serves multiple purposes:
 *
 *      - It can be used to perform an irregularly shaped blit more
 *        efficiently than by issuing many separate blit commands.
 *
 *      - It is equivalent to allowing blits with non-integer
 *        source coordinates. You could blit just one half-pixel
 *        of a source, for example, by specifying a larger
 *        destination rectangle than you need, then removing
 *        part of it using a clip rectangle.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT
 *
 * Limitations:
 *
 *    - Currently, no backend supports blits from a mipmap or face
 *      other than the first one.
 */

typedef
struct {
	SVGA3dSurfaceImageId srcImage;
	SVGASignedRect       srcRect;
	uint32_t               destScreenId; /* Screen ID or SVGA_ID_INVALID for virt. coords */
	SVGASignedRect       destRect;     /* Supports scaling if src/rest different size */
	/* Clipping: zero or more SVGASignedRects follow */
} SVGA3dCmdBlitSurfaceToScreen;         /* SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN */

typedef
struct {
	uint32_t               sid;
	SVGA3dTextureFilter  filter;
} SVGA3dCmdGenerateMipmaps;             /* SVGA_3D_CMD_GENERATE_MIPMAPS */


/*
 * Capability query index.
 *
 * Notes:
 *
 *   1. SVGA3D_DEVCAP_MAX_TEXTURES reflects the maximum number of
 *      fixed-function texture units available. Each of these units
 *      work in both FFP and Shader modes, and they support texture
 *      transforms and texture coordinates. The host may have additional
 *      texture image units that are only usable with shaders.
 *
 *   2. The BUFFER_FORMAT capabilities are deprecated, and they always
 *      return TRUE. Even on physical hardware that does not support
 *      these formats natively, the SVGA3D device will provide an emulation
 *      which should be invisible to the guest OS.
 *
 *      In general, the SVGA3D device should support any operation on
 *      any surface format, it just may perform some of these
 *      operations in software depending on the capabilities of the
 *      available physical hardware.
 *
 *      XXX: In the future, we will add capabilities that describe in
 *      detail what formats are supported in hardware for what kinds
 *      of operations.
 */

typedef enum {
	SVGA3D_DEVCAP_3D                                = 0,
	SVGA3D_DEVCAP_MAX_LIGHTS                        = 1,
	SVGA3D_DEVCAP_MAX_TEXTURES                      = 2,  /* See note (1) */
	SVGA3D_DEVCAP_MAX_CLIP_PLANES                   = 3,
	SVGA3D_DEVCAP_VERTEX_SHADER_VERSION             = 4,
	SVGA3D_DEVCAP_VERTEX_SHADER                     = 5,
	SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION           = 6,
	SVGA3D_DEVCAP_FRAGMENT_SHADER                   = 7,
	SVGA3D_DEVCAP_MAX_RENDER_TARGETS                = 8,
	SVGA3D_DEVCAP_S23E8_TEXTURES                    = 9,
	SVGA3D_DEVCAP_S10E5_TEXTURES                    = 10,
	SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND             = 11,
	SVGA3D_DEVCAP_D16_BUFFER_FORMAT                 = 12, /* See note (2) */
	SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT               = 13, /* See note (2) */
	SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT               = 14, /* See note (2) */
	SVGA3D_DEVCAP_QUERY_TYPES                       = 15,
	SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING         = 16,
	SVGA3D_DEVCAP_MAX_POINT_SIZE                    = 17,
	SVGA3D_DEVCAP_MAX_SHADER_TEXTURES               = 18,
	SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH                 = 19,
	SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT                = 20,
	SVGA3D_DEVCAP_MAX_VOLUME_EXTENT                 = 21,
	SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT                = 22,
	SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO          = 23,
	SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY            = 24,
	SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT               = 25,
	SVGA3D_DEVCAP_MAX_VERTEX_INDEX                  = 26,
	SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS    = 27,
	SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS  = 28,
	SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS           = 29,
	SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS         = 30,
	SVGA3D_DEVCAP_TEXTURE_OPS                       = 31,
	SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8               = 32,
	SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8               = 33,
	SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10            = 34,
	SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5               = 35,
	SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5               = 36,
	SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4               = 37,
	SVGA3D_DEVCAP_SURFACEFMT_R5G6B5                 = 38,
	SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16            = 39,
	SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8      = 40,
	SVGA3D_DEVCAP_SURFACEFMT_ALPHA8                 = 41,
	SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8             = 42,
	SVGA3D_DEVCAP_SURFACEFMT_Z_D16                  = 43,
	SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8                = 44,
	SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8                = 45,
	SVGA3D_DEVCAP_SURFACEFMT_DXT1                   = 46,
	SVGA3D_DEVCAP_SURFACEFMT_DXT2                   = 47,
	SVGA3D_DEVCAP_SURFACEFMT_DXT3                   = 48,
	SVGA3D_DEVCAP_SURFACEFMT_DXT4                   = 49,
	SVGA3D_DEVCAP_SURFACEFMT_DXT5                   = 50,
	SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8           = 51,
	SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10            = 52,
	SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8               = 53,
	SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8               = 54,
	SVGA3D_DEVCAP_SURFACEFMT_CxV8U8                 = 55,
	SVGA3D_DEVCAP_SURFACEFMT_R_S10E5                = 56,
	SVGA3D_DEVCAP_SURFACEFMT_R_S23E8                = 57,
	SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5               = 58,
	SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8               = 59,
	SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5             = 60,
	SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8             = 61,
	SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES        = 63,
	
	/*
	 * Note that MAX_SIMULTANEOUS_RENDER_TARGETS is a maximum count of color
	 * render targets.  This does no include the depth or stencil targets.
	 */
	SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS   = 64,
	
	SVGA3D_DEVCAP_SURFACEFMT_V16U16                 = 65,
	SVGA3D_DEVCAP_SURFACEFMT_G16R16                 = 66,
	SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16           = 67,
	SVGA3D_DEVCAP_SURFACEFMT_UYVY                   = 68,
	SVGA3D_DEVCAP_SURFACEFMT_YUY2                   = 69,
	SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES    = 70,
	SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES       = 71,
	SVGA3D_DEVCAP_ALPHATOCOVERAGE                   = 72,
	SVGA3D_DEVCAP_SUPERSAMPLE                       = 73,
	SVGA3D_DEVCAP_AUTOGENMIPMAPS                    = 74,
	SVGA3D_DEVCAP_SURFACEFMT_NV12                   = 75,
	SVGA3D_DEVCAP_SURFACEFMT_AYUV                   = 76,
	
	/*
	 * This is the maximum number of SVGA context IDs that the guest
	 * can define using SVGA_3D_CMD_CONTEXT_DEFINE.
	 */
	SVGA3D_DEVCAP_MAX_CONTEXT_IDS                   = 77,
	
	/*
	 * This is the maximum number of SVGA surface IDs that the guest
	 * can define using SVGA_3D_CMD_SURFACE_DEFINE*.
	 */
	SVGA3D_DEVCAP_MAX_SURFACE_IDS                   = 78,
	
	SVGA3D_DEVCAP_SURFACEFMT_Z_DF16                 = 79,
	SVGA3D_DEVCAP_SURFACEFMT_Z_DF24                 = 80,
	SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT            = 81,
	
	SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM              = 82,
	SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM              = 83,
	
	/*
	 * Don't add new caps into the previous section; the values in this
	 * enumeration must not change. You can put new values right before
	 * SVGA3D_DEVCAP_MAX.
	 */
	SVGA3D_DEVCAP_MAX                                  /* This must be the last index. */
} SVGA3dDevCapIndex;

typedef union {
	bool   b;
	uint32_t u;
	int32_t  i;
	float  f;
} SVGA3dDevCapResult;

#endif /* _SVGA3D_REG_CPP_ */

