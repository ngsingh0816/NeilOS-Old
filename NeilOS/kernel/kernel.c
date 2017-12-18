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
 * Ported GCC
 * Ported Dash
 */

/* TODO (could be improved):
 * Scheduler (round robin probably not the best) - could do something basic
	where you measure the time from one context switch to another (because processes
	that aren't CPU intensive will call schedule() and sleep so they will not use up
	their whole alloted time), and compare it to their alloted time. If they use all of it,
	give them more time next quantum (to a limit), otherwise give them less.
 * Signals - make the signal execute in user space, not kernel space (same with dylib init's)
 * Dynamic libraries - lazy linking, dynamic constructors / destructors
 * Disk Scheduling / Improvements
 */

/* TODO (bugs)
 * Caps lock can mess up dashes (-)
 * scheduler can take too long and have another interrupt start pending so that as soon as interrupts are enabled
	we go back to the scheduler
 * Get rid of all set_multitasking_enabled(false) because it can cause the disk spin lock to hang
	because a task will be reading something, we context switch into another task, which disables
	multitasking, and then tries to read but will just hang forever because the first task hasn't let go
	of the lock
 * Making fork disable multitasking should make the pipe test freeze, but it doesn't, it just outputs nothing
 * Sometimes control+c doesn't work - has to do with somehow multitasking getting disabled
 */

/* Things to test
 */

/* TODO:
 * More system calls - interrupt.c
 * (Kernel?) Threads (pthread?, gcd?)
	* Potential multitasking issues:
		* file locks
		* reading in one thread while closing in another (file descriptor lock)
		* Need a memory lock for all vm_get_next_unmapped_page
		* Probably should use locks instead of cli or set_multitasking(false) in most cases
		* Also definitely missing locks for just about everything (especially linked lists)
 * SMP?
 * Scheduler Rework
 * API (add user level support for all new features continuing - also make a user level program to test each of these functionalities)
 * Mouse Driver
 * Sound Drivers (Sound Blaster 16, Ensoniq AudioPCI ES1370?)
 * Graphics (VMWare) Driver (VMWare SVGA-II - can be used in qemu by doing -vga vmware)
 * Software Graphics Driver
 * Graphics (QEMU VBE) Driver?
 * GUI (Compositing Window Manager)
	* Interacts through message queues?
 * Ethernet Driver
 * Sockets
 * Message Queues?
 * Get libtool working?
 * Dylib Lazy Linking?
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
	terminal_init();			// Clears the screen
	rtc_init();
	
	// Initialize the filesystem and tasks
	sti();						// Needed for reading the files
	
	ata_init();
	filesystem_init("disk0s1");
	
	// Initialize the scheduler
	pit_init();
	pit_register_handler(schedule);
	
	devices_init();
	
	clear();
	
	// Basic file command prompt
	const char* shell_argv[] = { "shell", NULL };
	queue_task_load("shell", shell_argv, NULL, NULL);
	
	descriptors = (file_descriptor_t**)kmalloc(sizeof(file_descriptor_t*) * NUMBER_OF_DESCRIPTORS);
	memset(descriptors, 0, sizeof(file_descriptor_t*) * NUMBER_OF_DESCRIPTORS);
	descriptors[STDIN] = terminal_open("stdin", FILE_MODE_READ);
	descriptors[STDOUT] = terminal_open("stdout", FILE_MODE_WRITE);
	descriptors[STDERR] = terminal_open("stderr", FILE_MODE_WRITE);
	
	char input[128];
	for (;;) {
		printf("NeilOS> ");
		uint32_t num = terminal_read(STDIN, input, 128);
		input[num - 1] = 0;
		if (strncmp(input, "stat ", strlen("stat ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[5], FILE_MODE_READ, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			uint32_t size = fseek(&file, uint64_make(0, 0), SEEK_END).low;
			printf("Filename: %s\n", &input[5]);
			printf("Size: 0x%x bytes\n", size);
			ext_inode_t inode = ext2_open(&input[5]);
			printf("Inode: 0x%x\n", inode);
			printf("Blocks: ");
			int q;
			for (q = 0; q < 15; q++)
				printf("0x%x ", inode.info.blocks[q]);
			printf("\n");
			
			fclose(&file);
		}/* else if (strncmp(input, "ls", strlen("ls")) == 0) {
			char* name = "";
			if (strlen(input) > 2)
				name = &input[3];
			
			file_descriptor_t file;
			if (!fopen(name, FILE_MODE_READ, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			char buffer[256];
			uint32_t len = 0;
			while ((len = fread(buffer, 1, 256, &file)) != 0)
				printf("%s/%s\n", name, buffer);
			
			fclose(&file);
			
		}*//* else if (strncmp(input, "cat ", strlen("cat ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[4], FILE_MODE_READ, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			char buffer[256];
			uint32_t len = 0;
			while ((len = fread(buffer, 1, 256, &file)) != 0) {
				int z;
				for (z = 0; z < len; z++)
					printf("%c", buffer[z]);
			}
			
			fclose(&file);
			
		} */else if (strncmp(input, "append ", strlen("append ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[7], FILE_MODE_READ_WRITE, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			num = terminal_read(STDIN, input, 128);
			fseek(&file, uint64_make(0, 0), SEEK_END);
			fwrite(input, num, 1, &file);
			
			fclose(&file);
		} else if (strncmp(input, "touch ", strlen("touch ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[6], FILE_MODE_READ | FILE_MODE_CREATE, &file)) {
				printf("Could not create file.\n");
				continue;
			}
			fclose(&file);
		} else if (strncmp(input, "rm ", strlen("rm ")) == 0) {
			if (!fsunlink(&input[3]))
				printf("File not found.\n");
		} else if (strncmp(input, "link ", strlen("link ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[5], FILE_MODE_READ, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			char buffer[128];
			num = terminal_read(STDIN, buffer, 128);
			buffer[num - 1] = 0;
			
			if (flink(&file, buffer) < 0)
				printf("Could not rename %s to %s", &input[5], buffer);
			
			fclose(&file);
		} else if (strncmp(input, "mkdir ", strlen("mkdir ")) == 0) {
			file_descriptor_t file;
			if (fopen(&input[6], FILE_MODE_READ, &file)) {
				printf("File or directory already exists.\n");
				fclose(&file);
				continue;
			}
			
			int ret = fmkdir(&input[6]);
			if (ret < 0)
				printf("Could not create directory\n");
		} else if (strncmp(input, "rmdir ", strlen("rmdir ")) == 0) {
			if (!fsunlink(&input[6]))
				printf("File not found.\n");
		} else if (strncmp(input, "truncate ", strlen("truncate ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[9], FILE_MODE_READ_WRITE, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			num = terminal_read(STDIN, input, 128);
			input[num - 1] = 0;
			int32_t pos = strlen(input) - 1;
			bool hex = (pos > 2) && (input[0] == '0' && input[1] == 'x');
			int32_t start = hex ? 2 : 0;
			uint32_t value = 0;
			uint32_t base = hex ? 16 : 10;
			uint32_t bit = 1;
			bool valid = true;
			while (pos != (start - 1)) {
				uint32_t v = 0;
				if (input[pos] >= '0' && input[pos] <= '9')
					v = input[pos] - '0';
				else if (input[pos] >= 'a' && input[pos] <= 'f')
					v = input[pos] - 'a' + 0xA;
				else if (input[pos] >= 'A' && input[pos] <= 'F')
					v = input[pos] - 'A' + 0xA;
				else {
					valid = false;
					break;
				}
				
				value += bit * v;
				bit *= base;
				
				pos--;
			}
			
			if (!valid) {
				printf("Invalid argument.\n");
				fclose(&file);
				continue;
			}
			
			if (hex)
				printf("Truncating to size=0x%x\n", value);
			else
				printf("Truncating to size=%d\n", value);
			
			if (value != ftruncate(&file, uint64_make(0, value)).low)
				printf("Could not truncate correctly.\n");
			
			fclose(&file);
		} else if (strncmp(input, "blocks", strlen("blocks")) == 0) {
			printf("0x%x blocks remaining.\n", ext2_superblock()->free_block_count);
		} else if (strncmp(input, "inodes", strlen("inodes")) == 0) {
			printf("0x%x inodes remaining.\n", ext2_superblock()->free_inode_count);
		} else if (strncmp(input, "exit", strlen("exit")) == 0) {
			task_list_t* t = tasks->pcb->children;
			while (t) {
				if (t->pcb)
					t->pcb->parent = NULL;
				task_list_t* next = t->next;
				kfree(t);
				t = next;
			}
			tasks->pcb->children = NULL;
			break;
		} else if (strncmp(input, "time", strlen("time")) == 0) {
#include <common/time.h>
			date_t date = get_current_date();
			printf("%d:%d:%d on %d/%d/%d\n", date.hour, date.minute, date.second, date.month, date.day, date.year);
			
			printf("UNIX time: %d\n", get_current_time().val);
		} else {
			if (num == 1)
				continue;
			
			uint32_t argc = 1;
			uint32_t ptr;
			uint32_t first = num - 1;
			bool was_space = false;
			for (ptr = 0; ptr < num; ptr++) {
				if (input[ptr] == ' ') {
					first = ptr;
					break;
				}
			}
			for (ptr = 0; ptr < num; ptr++) {
				if (input[ptr] == ' ') {
					was_space = true;
				} else if (was_space) {
					argc++;
					was_space = false;
				}
			}
			
			char** argv = (char**)kmalloc(sizeof(char*) * (argc + 1));
			argv[argc] = NULL;
			argv[0] = (char*)kmalloc(first + 1);
			memcpy(argv[0], input, first);
			argv[0][first] = 0;
			
			argc = 0;
			for (ptr = first; ptr < num; ptr++) {
				if (input[ptr] == ' ') {
					was_space = true;
				} else if (was_space) {
					argc++;
					was_space = false;
					uint32_t z = 0;
					for (z = 0; ptr + z < num; z++) {
						if (input[ptr + z] == ' ')
							break;
					}
					argv[argc] = (char*)kmalloc(z + 1);
					argv[argc][z] = 0;
					memcpy(argv[argc], &input[ptr], z);
				}
			}
			
			pcb_t* p = NULL;
			int res = queue_task_load(argv[0], (const char**)argv, NULL, &p);
			uint32_t z;
			for (z = 0; z < argc; z++)
				kfree(argv[z]);
			kfree(argv);
			if (res == 0) {
				p->parent = tasks->pcb;
				run(p);
				signal_set_pending(tasks->pcb, SIGCHLD, false);
			}
			else
				printf("Command not found.\n");
		}
	}
	
	// Run the first shell (auto enables interrupts)
	run(pcb_from_pid(0));
	
	// Avoids error in Xcode
#ifndef __APPLE__
	// Spin (nicely, so we don't chew up cycles)
	asm volatile(".1: hlt; jmp .1;");
#endif
}
