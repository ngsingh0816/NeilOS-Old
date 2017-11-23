//
//  crt0.c
//  
//
//  Created by Neil Singh on 6/22/17.
//
//

extern void main();
extern void _init();

static char* initial_environ[] = { (char*)0 };
char** environ = &initial_environ[0];

extern void (*__preinit_array_start []) (void) __attribute__((weak));
extern void (*__preinit_array_end []) (void) __attribute__((weak));
extern void (*__init_array_start []) (void) __attribute__((weak));
extern void (*__init_array_end []) (void) __attribute__((weak));

void _init_arrays() {
	unsigned int count = __preinit_array_end - __preinit_array_start;
	for (unsigned int z = 0; z < count; z++)
		__preinit_array_start[z]();
	
	_init();
	
	count = __init_array_end - __init_array_start;
	for (unsigned int z = 0; z < count; z++)
		__init_array_start[z]();
}

void _load_environment(int argc, char** argv, char** envp) {
	environ = envp;
}

asm(".globl _start\n"
	"_start: \n"
	// Initialize globals
	"call _init_arrays\n"
	
	// Get the correct environment variables
	"call _load_environment\n"

	// argc, argv, envp implicity pushed by kernel
	"call main\n"
	"addl $12, %esp\n"
	"pushl $0\n"
	"pushl $0\n"
	"pushl %eax\n"

	// Exit the program
	"call _exit\n"
);
