/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include <boot/multiboot.h>
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <drivers/pic/i8259.h>
#include <drivers/pci/pci.h>
#include <common/debug.h>
#include <syscalls/interrupt.h>
#include <drivers/keyboard/keyboard.h>
#include <memory/memory.h>
#include <drivers/rtc/rtc.h>
#include <drivers/ATA/ata.h>
#include <drivers/filesystem/filesystem.h>
#include <drivers/terminal/terminal.h>
#include <drivers/pit/pit.h>
#include <program/task.h>
#include <drivers/devices/devices.h>
#include <drivers/mouse/mouse.h>
#include <drivers/audio/es1371.h>
#include <drivers/graphics/graphics.h>

/* Features:
 * Memory Allocator
 * Locks (spinlock, mutex, semaphore, r/w)
 * Hard Drive Driver
 * Scheduler
 * Ext2 Read Support
 * Ext2 Write Support
 * Time
 * Virtual Memory Manager (bitmap for 4mb pages)
 * Elf file loading
 * libc (static newlib)
 * Signals
 * C++ User support
 * Pipes
 * Named Pipes (FIFOs)
 * Logging
 * Dynamic Libraries
 * Copy on Write
 * Grub2
 * Higher Half Kernel
 * Ported GCC / Dash / Bash / etc.
 * User Threads (pthreads)
 * Mouse Driver
 * Audio Driver (Ensoniq AudioPCI 1370/1371)
 * mmap (also munmap, msync)
 * Posix Shared Memory
 * Posix Message Queues
 * API
 * VMWare Graphics Driver (2D and 3D)
 */

/* TODO (could be improved):
 * Scheduler (round robin probably not the best) - could do something basic
	where you measure the time from one context switch to another (because processes
	that aren't CPU intensive will call schedule() and sleep so they will not use up
	their whole alloted time), and compare it to their alloted time. If they use all of it,
	give them more time next quantum (to a limit), otherwise give them less.
 	* Process / thread user priorities?
 * Signals - make the signal execute in user space, not kernel space (same with dylib init's)
 * Dynamic libraries - lazy linking, dynamic constructors / destructors
 * Disk Scheduling / Improvements
 * Kernel Threads
 * Filesystem - multiple processes or threads writing to the same inode (i.e. fmkdir("/hi") and fmkdir("/bye")
	may cause an issue because they both modify the "." inode's contents at the same time
 		* can probably be fixed by r/w lock per inode
 * Implement MS_INVALIDATE for msync (needs way to easily located all other mapped versions of the same file^^^^)
 * Thread Local Storage (TLS)
 * For shared memory and mqueues, could make a /dev/mqueue/ directory and put all open mqueues in there
 * Could improve read speed of large reads by reading entire block and caching it (because fread only
 	does 0x400 at a time)
 * Make libpthread work with -O3
 * Make pthread_cond variables put the thread to sleep in the kernel (also for NSConditionalLock)
 	* needs some syscalls like thread_pause, thread_continue
 * Make asynchrous version of graphics3d_surface_dma and make it actually alloc GMR regions
 */

/* TODO (bugs)
 * bin/dash -> control-c -> ls crashes
 	* This is because the signal handler executes in kernel space for now, and dash does a longjmp to
 		get out of the signal, so it remains in kernel mode for dash execution so then calls to intx80
 		don't use the correct esp because there is no ring switch
 * bin/dash -> / -> / -> etc, eventually crashes
 * bin/bash crashes if TERM!=dumb
 * Implement CLOSE_ON_EXEC
 * cat "binary file" crashes because it tries to output nonexistant escape sequences
 */

/* TODO (API)
 * NSTimer, NSThread (NSLock, etc.) -> NSHandler
 	* NSHandler can be a way to post handlers to a certain thread (ex: from background thread,
 		handler->post(NSThread::mainThread())
 		* This means that each NSThread must have its own NSRunLoop
	 * Event handling?
	 NSEvent?, Run loops?????
 * GUI
 	* NSPoint, NSSize, NSRect, NSRange
	* NSWindow, NSView, NSControl, NSButton, etc.
 * NSUndoManager
 * Implement sockets
 	* NSURL*
 
 * Implement NSImage (TIFF and GIF)?
 */

/* TODO:
 * GUI (Compositing Window Manager) - http://designmodo.github.io/Flat-UI/
	* Interacts through message queues
 	How it will work
 		1) Client requests access to WindowServer, WindowServer opens message queue and returns it
 		2) Client requests window, WindowServer sets up shared memory for window and buffer
		3) Client will draw it things into buffer, sends Update Rect to WindowServer, which composites results to screen
 			* View writes into window's buffer, then sends update request to window, which sends to WindowServer
 		* Every application will have personal message queue, where other processes send a message to the app's message
 			queue and the application reads those messages. This supports things such as opening files with certain apps,
 			click + drag, etc. The WindowServer will keep track of which application has the focused window and will only
 			keep that application's message queue open so that it can send events to the focused application. The message
 			queue will be named by the process's pid
 * FIX ALL BUGS (TODO bugs and could be improved)
 * Scheduler Rework
 * Ethernet Driver
 * Sockets
 * Open vm-tools
 	* Mouse automatically capturing in VMWare (and shared files??)
 * Dylib Lazy Linking?
 * Get libtool working?
 * SMP?
 * Page files on disk?
	* Could make loading a program only load entry point page and then lazy load the rest?
 * Module support?
 * OpenGL (Mesa - has software rendering and could implement hardware driver, TinyGL - only software rendering)?
 * USB Driver?
 * NFS Driver?
 * Wifi Driver?
 * Bluetooth Driver?
 * Bootloader?
 * Internet Browser?
 */

void
entry (unsigned long magic, unsigned long addr)
{
	// Paging has already been set up by the time we get here
	complete_paging_setup();
	
	clear();
	
	// Setup our page allocator
	page_allocator_init();
	
	/* Construct an LDT entry in the GDT */
	{
		seg_desc_t the_ldt_desc;
		the_ldt_desc.granularity    = 0;
		the_ldt_desc.opsize         = 1;
		the_ldt_desc.reserved       = 0;
		the_ldt_desc.avail          = 0;
		the_ldt_desc.present        = 1;
		the_ldt_desc.dpl            = 0x0;
		the_ldt_desc.sys            = 0;
		the_ldt_desc.type           = 0x2;
		
		SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
		ldt_desc_ptr = the_ldt_desc;
		lldt(KERNEL_LDT);
	}
	
	/* Construct a TSS entry in the GDT */
	{
		seg_desc_t the_tss_desc;
		the_tss_desc.granularity    = 0;
		the_tss_desc.opsize         = 0;
		the_tss_desc.reserved       = 0;
		the_tss_desc.avail          = 0;
		the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
		the_tss_desc.present        = 1;
		the_tss_desc.dpl            = 0x0;
		the_tss_desc.sys            = 0;
		the_tss_desc.type           = 0x9;
		the_tss_desc.seg_lim_15_00  = TSS_SIZE & 0x0000FFFF;
		
		SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);
		
		tss_desc_ptr = the_tss_desc;
		
		tss.ldt_segment_selector = KERNEL_LDT;
		tss.ss0 = KERNEL_DS;
		tss.esp0 = 0x800000;
		ltr(KERNEL_TSS);
	}
	
	// Disable interrupts
	cli();
	
	/* Init the PIC */
	i8259_init();
	
	// Load the IDT
	load_idt();
	
	/* Initialize devices, memory, filesystem, enable device interrupts on the
	 * PIC, any other initialization stuff... */
	
	// Initialize some drivers
	keyboard_init();
	mouse_init();
	terminal_init();			// Clears the screen
	rtc_init();
	
	// Initialize the filesystem and tasks
	sti();						// Needed for reading the files
	
	ata_init();
	filesystem_init("disk0s1");
	es_init();
	
	// Initialize the scheduler
	pit_init();
	time_load_current();
	pit_register_handler(scheduler_tick);
	
	devices_init();
	
	clear();
	
	// Setup the graphics mode
	pcb_t* pcb = NULL;
	if (!graphics_init()) {
		// Start with bash if graphics aren't supported
		const char* shell_argv[] = { "bash", NULL };
		queue_task_load("/bin/bash", shell_argv, NULL, &pcb);
	} else {
		// Otherwise, start the GUI
		const char* server_argv[] = { "WindowServer", NULL };
		queue_task_load("/system/apps/WindowServer", server_argv, NULL, &pcb);
	}
	
	// Run the first program (auto enables interrupts)
	run(pcb);
	
	// Avoids error in Xcode
#ifndef __APPLE__
	asm volatile(".1: hlt; jmp .1;");
#endif
}
