/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include <boot/multiboot.h>
#include <boot/x86_desc.h>
#include <common/lib.h>
#include <drivers/i8259.h>
#include <drivers/pci.h>
#include <common/debug.h>
#include <syscalls/interrupt.h>
#include <drivers/keyboard/keyboard.h>
#include <memory/memory.h>
#include <drivers/rtc/rtc.h>
#include <drivers/ATA/ata.h>
#include <drivers/filesystem/filesystem.h>
#include <drivers/terminal/terminal.h>
#include <syscalls/syscall.h>
#include <drivers/pit/pit.h>
#include <program/task.h>

/* Done:
 * Memory Allocator
 * Locks (spinlock, mutex, semaphore, r/w)
 * Ext2 Read Support
 * Ext2 Write Support
 * Time
 * Virtual Memory Manager (bitmap for 4mb pages)
 * Redo syscalls to be more linux / Create own (basic) programs
 * Elf file format
 */

/* Done (but could be improved):
 * Hard Drive Driver (has VMWare DMA bug)
	* Have variable ATA block size (so we can set it to the EXT2 block size) for optimization purposes
 * Rework syscall open - need to dynamically parse devices
 * Scheduler (round robin probably not the best) - could do something basic
	where you measure the time from one context switch to another (because processes
	that aren't CPU intensive will call schedule() and sleep so they will not use up
	their whole alloted time), and compare it to their alloted time. If they use all of it,
	give them more time next quantum (to a limit), otherwise give them less.
 * More system calls - sleep (nanosleep)
 * Signals - make the signal execute in user space, not kernel space
 */

/* TODO (bugs)
 */

/* TODO:
 * Newlib (static) / Redo and create more programs
 * (Named) Pipes
 * Sockets?
 * (Kernal?) Threads (pthread?)
 * Shared User Libraries (dynamic newlib)
 * Page files on disk?
 * C++ User support
 * API (add user level support for all new features continuing - also make a user level program to test each of these functionalities)
 * Mouse Driver
 * Sound Drivers (Sound Blaster 16, Ensoniq AudioPCI ES1370?)
 * Ethernet Driver
 * Graphics (VMWare) Driver (VMWare SVGA-II - can be used in qemu by doing -vga vmware)
 * Grahpics (QEMU VBE) Driver?
 * GUI (Compositing Window Manager)
 * Improved Scheduler
 * SMP?
 * OpenGL (Mesa - has software rendering and could implement hardware driver, TinyGL - only software rendering)?
 * USB Driver?
 * NFS Driver?
 * Wifi Driver?
 * Bluetooth Driver?
 * Bootloader?
 * Internet Browser?
 */

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
 pointed by ADDR. */
void
entry (unsigned long magic, unsigned long addr)
{
	multiboot_info_t *mbi;
		
	/* Clear the screen. */
	clear();
	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		printf ("Invalid magic number: 0x%#x\n", (unsigned) magic);
		return;
	}
	
	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;
	
	/* Print out the flags. */
	printf ("flags = 0x%#x\n", (unsigned) mbi->flags);
	
	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
		printf ("mem_lower = %uKB, mem_upper = %uKB\n",
				(unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);
	
	/* Is boot_device valid? */
	if (CHECK_FLAG (mbi->flags, 1))
		printf ("boot_device = 0x%#x\n", (unsigned) mbi->boot_device);
	
	/* Is the command line passed? */
	if (CHECK_FLAG (mbi->flags, 2))
		printf ("cmdline = %s\n", (char *) mbi->cmdline);
	
	if (CHECK_FLAG (mbi->flags, 3)) {
		int mod_count = 0;
		int i;
		module_t* mod = (module_t*)mbi->mods_addr;
		while(mod_count < mbi->mods_count) {
			printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
			printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
			printf("First few bytes of module:\n");
			for(i = 0; i<16; i++) {
				printf("0x%x ", *((char*)(mod->mod_start+i)));
			}
			printf("\n");
			
			mod_count++;
			mod++;
		}
	}
	/* Bits 4 and 5 are mutually exclusive! */
	if (CHECK_FLAG (mbi->flags, 4) && CHECK_FLAG (mbi->flags, 5))
	{
		printf ("Both bits 4 and 5 are set.\n");
		return;
	}
	
	/* Is the section header table of ELF valid? */
	if (CHECK_FLAG (mbi->flags, 5))
	{
		elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
		
		printf ("elf_sec: num = %u, size = 0x%#x,"
				" addr = 0x%#x, shndx = 0x%#x\n",
				(unsigned) elf_sec->num, (unsigned) elf_sec->size,
				(unsigned) elf_sec->addr, (unsigned) elf_sec->shndx);
	}
	
	/* Are mmap_* valid? */
	if (CHECK_FLAG (mbi->flags, 6))
	{
		memory_map_t *mmap;
		
		printf ("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
				(unsigned) mbi->mmap_addr, (unsigned) mbi->mmap_length);
		for (mmap = (memory_map_t *) mbi->mmap_addr;
				(unsigned long) mmap < mbi->mmap_addr + mbi->mmap_length;
				mmap = (memory_map_t *) ((unsigned long) mmap
										 + mmap->size + sizeof (mmap->size)))
			printf (" size = 0x%x,     base_addr = 0x%#x%#x\n"
					"     type = 0x%x,  length    = 0x%#x%#x\n",
					(unsigned) mmap->size,
					(unsigned) mmap->base_addr_high,
					(unsigned) mmap->base_addr_low,
					(unsigned) mmap->type,
					(unsigned) mmap->length_high,
					(unsigned) mmap->length_low);
	}
	
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
	
	//clear();
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
	
	// Setup our page allocator
	page_allocator_init();
	
	// Setup paging
	setup_pages();
	
	// Initialize the filesystem and tasks
	sti();						// Needed for reading the files
	
	ata_init();
	filesystem_init("disk0s1");
	
	// Initialize the scheduler
	pit_init();
	pit_register_handler(schedule);
	
	clear();
	
	// Basic file command prompt
	const char* shell_argv[] = { "shell", NULL };
	queue_task_load("shell", shell_argv, NULL, NULL);

	descriptors = (file_descriptor_t**)kmalloc(sizeof(file_descriptor_t*) * NUMBER_OF_DESCRIPTORS);
	memset(descriptors, 0, sizeof(file_descriptor_t*) * NUMBER_OF_DESCRIPTORS);
	descriptors[STDIN] = terminal_open("stdin", FILE_MODE_READ);
	descriptors[STDOUT] = terminal_open("stdout", FILE_MODE_WRITE);
	
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
			file_descriptor_t file;
			if (!fopen(&input[3], FILE_MODE_READ | FILE_MODE_DELETE_ON_CLOSE, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			if (!fclose(&file))
				printf("Could not delete file.\n");
		} else if (strncmp(input, "link ", strlen("link ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[5], FILE_MODE_READ, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			char buffer[128];
			num = terminal_read(STDIN, buffer, 128);
			buffer[num - 1] = 0;
			
			if (!flink(&file, buffer))
				printf("Could not rename %s to %s", &input[5], buffer);
			
			fclose(&file);
		} else if (strncmp(input, "mkdir ", strlen("mkdir ")) == 0) {
			file_descriptor_t file;
			if (fopen(&input[3], FILE_MODE_READ, &file)) {
				printf("File or directory already exists.\n");
				fclose(&file);
				continue;
			}
			
			bool ret = fmkdir(&input[6]);
			if (!ret)
				printf("Could not create directory\n");
		} else if (strncmp(input, "rmdir ", strlen("rmdir ")) == 0) {
			file_descriptor_t file;
			if (!fopen(&input[6], FILE_MODE_READ | FILE_MODE_DELETE_ON_CLOSE, &file)) {
				printf("File not found.\n");
				continue;
			}
			
			if (!fclose(&file))
				printf("Could not delete directory.\n");
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
