#include "terminal.h"
#include <drivers/keyboard/keyboard.h>
#include <common/lib.h>

#include <drivers/filesystem/filesystem.h>
#include <program/task.h>
#include <memory/memory.h>
#include <common/log.h>
#include <syscalls/interrupt.h>

// Terminal options
termios_t termios;

uint8_t buffer[MAX_TERMINAL_BUFFER_LENGTH];
// This is the buffer write position (tail of circular buffer)
uint32_t buffer_tail = 0;
// Head of circular buffer
uint32_t buffer_head = 0;
uint32_t buffer_size = 0;

//array of shifted values
uint8_t shift_map[] = {
	')', '!', '@', '#', '$', '%', '^', '&', '*', '('
};

//takes in a key and turns it into its shifted value
//inputs: keycode
//outputs: keycode's shifted value
uint8_t handle_shift(uint8_t key) {
	if (key >= '0' && key <= '9')
		return shift_map[key - '0'];
	if (key >= 'a' && key <= 'z')
		return key - 'a' + 'A';
	if (key >= 'A' && key <= 'Z')
		return key - 'A' + 'a';
	switch (key) {
		case '`':
			return '~';
		case '-':
			return '_';
		case '=':
			return '+';
		case '[':
			return '{';
		case ']':
			return '}';
		case '\\':
			return '|';
		case '\'':
			return '"';
		case ';':
			return ':';
		case ',':
			return '<';
		case '.':
			return '>';
		case '/':
			return '?';
	}
	
	return key;
}

//takes in a key and turns it into its uppercase value
//inputs: keycode
//outputs: keycode's uppercase value
uint8_t to_upper(uint8_t key) {
	if (key >= 'a' && key <= 'z')
		return key - 'a' + 'A';
	return key;
}

uint32_t circular_dec(uint32_t* d, uint32_t len) {
	if ((*d)-- == 0)
		*d = len - 1;
	return *d;
}

uint32_t circular_inc(uint32_t* d, uint32_t len) {
	uint32_t old = *d;
	if (++(*d) >= len)
		*d = 0;
	return old;
}

static inline bool test_for_enter() {
	uint32_t copy = buffer_tail;
	circular_dec(&copy, MAX_TERMINAL_BUFFER_LENGTH);
	return (buffer[copy] == ENTER_KEY || buffer[copy] == 0);
}

// Handle a control being pressed
bool handle_control(uint8_t* keycode, modifier_keys_t modifier_keys) {
	uint8_t c = to_upper(*keycode) - 'A' + 1;
	if (c == termios.cc[VEOF] && (termios.lflag & ICANON)) {
		if (buffer_size != MAX_TERMINAL_BUFFER_LENGTH) {
			buffer[circular_inc(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)] = EOF_KEY;
			buffer_size++;
		}
		return true;
	} else if (c == termios.cc[VEOL] && (termios.lflag & ICANON)) {
		if (buffer_size != MAX_TERMINAL_BUFFER_LENGTH) {
			buffer[circular_inc(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)] = 0x0;
			buffer_size++;
		}
		return true;
	} else if (c == termios.cc[VERASE] && (termios.lflag & ICANON) && (termios.lflag & ECHOE)) {
		if (buffer_size != 0 && !test_for_enter()) {
			backspace(buffer[circular_dec(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)]);
			buffer_size--;
		}
		return true;
	} else if (c == termios.cc[VINTR] && (termios.lflag & ISIG)) {
		task_list_t* t = tasks;
		task_list_t* prev = tasks;
		while (t) {
			prev = t;
			t = t->next;
		}
		if (prev && prev->pcb)
			signal_send(prev->pcb, SIGINT);
		if (!(termios.lflag & NOFLSH)) {
			buffer_head = 0;
			buffer_tail = 0;
			buffer_size = 0;
		}
		return true;
	} else if (c == termios.cc[VKILL] && (termios.lflag & ICANON) && (termios.lflag & ECHOK)) {
		while (buffer_tail != buffer_head) {
			if (test_for_enter())
				return true;
			backspace(buffer[circular_dec(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)]);
			buffer_size--;
		}
		buffer_head = 0;
		buffer_tail	= 0;
		buffer_size = 0;
		return true;
	} else if (c == termios.cc[VQUIT] && (termios.lflag & ISIG)) {
		task_list_t* t = tasks;
		task_list_t* prev = tasks;
		while (t) {
			prev = t;
			t = t->next;
		}
		if (prev && prev->pcb)
			signal_send(prev->pcb, SIGQUIT);
		if (!(termios.lflag & NOFLSH)) {
			buffer_head = 0;
			buffer_tail = 0;
			buffer_size = 0;
		}
		return true;
	} else if (c == termios.cc[VSTART] && (termios.iflag & IXON)) {
		return true;
	} else if (c == termios.cc[VSTOP] && (termios.iflag & IXON)) {
		return true;
	} else if (c == termios.cc[VQUIT] && (termios.lflag & ISIG)) {
		task_list_t* t = tasks;
		task_list_t* prev = tasks;
		while (t) {
			prev = t;
			t = t->next;
		}
		if (prev && prev->pcb)
			signal_send(prev->pcb, SIGTSTP);
		if (!(termios.lflag & NOFLSH)) {
			buffer_head = 0;
			buffer_tail = 0;
			buffer_size = 0;
		}
		return true;
	}
	
	if (!(termios.lflag & ICANON))
		return false;
	return true;
}

int get_escape_code(uint8_t keycode, uint8_t* bytes, char* print) {
	switch (keycode) {
		case UP_ARROW_KEY:
		case DOWN_ARROW_KEY:
		case RIGHT_ARROW_KEY:
		case LEFT_ARROW_KEY:
			bytes[0] = 0x1B;
			bytes[1] = '[';
			bytes[2] = keycode - UP_ARROW_KEY + 'A';
			strcpy(print, "^[[A");
			print[3] = keycode - UP_ARROW_KEY + 'A';
			return 3;
		case F1_KEY:
		case F2_KEY:
		case F3_KEY:
		case F4_KEY:
		case F5_KEY:
		case F6_KEY:
		case F7_KEY:
		case F8_KEY:
		case F9_KEY:
		case F10_KEY:
		case F11_KEY:
		case F12_KEY:
			bytes[0] = 0x1B;
			bytes[1] = 'O';
			bytes[2] = keycode - F1_KEY + 'P';
			strcpy(print, "^[OP");
			print[3] = keycode - F1_KEY + 'P';
			return 3;
		case TAB_KEY:
			bytes[0] = '\t';
			strcpy(print, " ");
			return 1;
	}
	
	return 0;
}

void handle_escape_sequences(uint8_t* buf, uint32_t* i, uint32_t len, bool csi) {
	uint32_t params[16];
	uint32_t pos = 0;
	uint32_t value = 0;
	bool had_char = false;
	while (*i < len) {
		uint8_t c = buf[*i];
		// Check for new escape sequences
		bool found = false;
		switch (c) {
			case ESCAPE_KEY:
				// Restart
				pos = 0;
				(*i)++;
				found = true;
				had_char = false;
				value = 0;
				break;
			case CSI_KEY:
				// Restart as CSI
				pos = 0;
				csi = true;
				(*i)++;
				found = true;
				had_char = false;
				value = 0;
				break;
			default: {
				if (c == '[' && pos == 0) {
					(*i)++;
					csi = true;
					found = true;
				} else if (c == '[' && pos == 1) {
					*i += 2;
					return;
				}
				break;
			}
		}
		if (found)
			continue;
		if (!csi) {
			switch (c) {
				case 'c':		// Reset
					(*i)++;
					clear();
					return;
				case 'D':		// Linefeed
				case 'E':		// Newline
					putc('\n');
					(*i)++;
					return;
				case 'M':		// Reverse linefeed
					reverse_line_feed();
					cursor_position_refresh();
					(*i)++;
					return;
				default:
					(*i)++;
					return;
			}
		} else {
			(*i)++;
			if (had_char)
				params[pos++] = value;
			if (c >= 0x40 && c <= 0x7E) {
				// Final byte
				switch (c) {
					// Cursor Up, Down, Right, Left
					case 'A': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set_y(cursor_position_get_y() - n);
						cursor_position_refresh();
						return;
					}
					case 'e':
					case 'B': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set_y(cursor_position_get_y() + n);
						cursor_position_refresh();
						return;
					}
					case 'a':
					case 'C': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set_x(cursor_position_get_x() + n);
						cursor_position_refresh();
						return;
					}
					case 'D': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set_x(cursor_position_get_x() - n);
						cursor_position_refresh();
						return;
					}
					// Set cursor begining of line up / down
					case 'E': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set(0, cursor_position_get_y() - n);
						cursor_position_refresh();
						return;
					}
					case 'F': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set(0, cursor_position_get_y() + n);
						cursor_position_refresh();
						return;
					}
					// Move cursor to column n
					case 'G': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set_x(n - 1);
						cursor_position_refresh();
						return;
					}
					// Move cursor to row n
					case 'd': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						cursor_position_set_y(n - 1);
						cursor_position_refresh();
						return;
					}
					// Move cursor to (n, m)
					case 'f':
					case 'H': {
						int n = 1, m = 1;
						if (pos > 0)
							n = params[0];
						if (pos > 1)
							m = params[1];
						cursor_position_set(n - 1, m - 1);
						return;
					}
					// Erase display
					case 'J': {
						int n = 0;
						if (pos > 0)
							n = params[0];
						clear_display(n);
						return;
					}
					// Erase line
					case 'K': {
						int n = 0;
						if (pos > 0)
							n = params[0];
						clear_line(n);
						return;
					}
					// Insert n lines
					case 'L': {
						int n = 1;
						if (pos > 0)
							n = params[0];
						for (int z = 0; z < n; z++)
							putc('\n');
						cursor_position_refresh();
						return;
					}
					case 'h':	// Set mode
					case 'l':	// Reset mode
					case 'r':	// Set scrolling region
					case 'm':	// Set attributes
						return;
					default: {
						printf("Unhandled CSI %c\n", c);
						return;
					}
				}
			} else {
				if (c == ';') {
					had_char = true;
					params[pos++] = value;
					value = 0;
					if (pos == 16)
						return;
				} else if (c == '?') {
					(*i)++;
				} else {
					value = value * 10 + (c - '0');
					had_char = true;
				}
			}
		}
	}
}

bool handle_control_chars(uint8_t* buf, uint32_t* i, uint32_t len) {
	switch (buf[*i]) {
		case NUL_KEY:
		case SHIFT_IN_KEY:
		case SHIFT_OUT_KEY:
		case CANCEL_KEY:
		case SUBSTITUTE_KEY:
		case DELETE_KEY:
			// Ignore it
			(*i)++;
			return true;
		case BEL_KEY:
			// Beep, but just ignore
			(*i)++;
			return true;
		case BACKSPACE_KEY:
			if (cursor_position_get_x() != 0)
				backspace(-0x40);
			(*i)++;
			return true;
		case TAB_KEY:
			putc(' ');
			(*i)++;
			return true;
		case ENTER_KEY:
		case VERTICAL_TAB_KEY:
		case NEW_PAGE_KEY:
			putc('\n');
			(*i)++;
			return true;
		case CARRIAGE_RETURN_KEY:
			cursor_position_set_x(0);
			(*i)++;
			return true;
		case ESCAPE_KEY:
			(*i)++;
			handle_escape_sequences(buf, i, len, false);
			return true;
		case CSI_KEY:
			(*i)++;
			handle_escape_sequences(buf, i, len, true);
			return true;
	}
	return false;
}

//takes in a key and converts it to its correct modified value, prints it to the screen and stores it in a buffer
//inputs: the keycode, the modified keys structure, and whether the current key is pressed or not
//outputs: none
void handle_keys(uint8_t keycode, modifier_keys_t modifier_keys, bool pressed) {
	// We only care about key pressed and if we have room in the buffer
	if (!pressed)
		return;
	
	bool has_shift = (modifier_keys & RIGHT_SHIFT) || (modifier_keys & LEFT_SHIFT);
	bool has_control = (modifier_keys & RIGHT_CONTROL) || (modifier_keys & LEFT_CONTROL);
	if (has_control) {
		if (!handle_control(&keycode, modifier_keys) && (termios.lflag & ECHO)) {
			if (buffer_size != MAX_TERMINAL_BUFFER_LENGTH) {
				buffer[circular_inc(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)] = keycode;
				putc('^');
				putc(keycode + 0x40);
				cursor_position_refresh();
			}
		}
	} else {
		if (keycode == BACKSPACE_KEY) {
			if ((termios.lflag & ECHOE) && buffer_size != 0 && !test_for_enter()) {
				backspace(buffer[circular_dec(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)]);
				buffer_size--;
			}
			if (termios.lflag & ICANON)
				return;
		} else if (keycode == CARRIAGE_RETURN_KEY) {
			if ((termios.iflag & IGNCR))
				return;
			if ((termios.iflag & ICRNL))
				keycode = ENTER_KEY;
		}
		uint8_t bytes[4];
		char print[5] = { 0, 0, 0, 0, 0 };
		int len = get_escape_code(keycode, bytes, print);
		if (len != 0) {
			if (len + buffer_size <= MAX_TERMINAL_BUFFER_LENGTH) {
				for (int z = 0; z < len; z++) {
					buffer[circular_inc(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)] = bytes[z];
					buffer_size++;
				}
				if ((termios.lflag & ECHO)) {
					puts(print);
					cursor_position_refresh();
				}
			}
			return;
		}
		
		if (keycode == LEFT_SHIFT_KEY || keycode == RIGHT_SHIFT_KEY || keycode == CAPS_LOCK_KEY ||
			keycode == ESCAPE_KEY || keycode == LEFT_ALT_KEY || keycode == RIGHT_ALT_KEY)
			return;
		
		// Check shift and caps lock
		if (modifier_keys & CAPS_LOCK)
			keycode = to_upper(keycode);
		if (has_shift)
			keycode = handle_shift(keycode);
		
		if (buffer_size < MAX_TERMINAL_BUFFER_LENGTH) {
			buffer[circular_inc(&buffer_tail, MAX_TERMINAL_BUFFER_LENGTH)] = keycode;
			buffer_size++;
			if ((termios.lflag & ECHO) || ((termios.lflag & ECHONL) && keycode == ENTER_KEY)) {
				putc(keycode);
				cursor_position_refresh();
			}
		}
	}
}

void terminal_init() {
	// Clear the buffer
	memset(buffer, 0, MAX_TERMINAL_BUFFER_LENGTH);
	buffer_head = buffer_tail = buffer_size = 0;
	
	// Clear the screen
	clear();
	
	// Register keyboard events
	register_keychange(handle_keys);
	
	// Setup the default termios
	memset(&termios, 0, sizeof(termios_t));
	termios.iflag = IGNBRK | ICRNL;
	termios.oflag = OPOST;
	termios.cflag = 0;
	termios.lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK;
	termios.cc[VEOF] = 4;					// ^D
	termios.cc[VEOL] = 0;					// NUL
	termios.cc[VERASE] = BACKSPACE_KEY;		// ^H
	termios.cc[VINTR] = 3;					// ^C
	termios.cc[VKILL] = 21;					// ^U
	termios.cc[VMIN] = 1;
	termios.cc[VQUIT] = 28;					// ^"\"
	termios.cc[VSTART] = 17;				// ^Q
	termios.cc[VSTOP] = 19;					// ^S
	termios.cc[VSUSP] = 26;					// ^Z
	termios.cc[VTIME] = 0;
	termios.ospeed = 9600;
	termios.ispeed = 9600;
}

// Initialize the terminal
//inputs: the filename
//outputs: -1 if failed or stdout if success
file_descriptor_t* terminal_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = STANDARD_INOUT_TYPE;
	d->mode = FILE_MODE_READ | FILE_MODE_WRITE | FILE_TYPE_CHARACTER;
	int namelen = strlen(filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, filename, namelen + 1);
	
	// Assign the functions
	d->read = terminal_read;
	d->write = terminal_write;
	d->stat = terminal_stat;
	d->llseek = terminal_llseek;
	d->ioctl = terminal_ioctl;
	d->can_read = terminal_can_read;
	d->can_write = terminal_can_write;
	d->duplicate = terminal_duplicate;
	d->close = terminal_close;
	
	return d;
}

uint32_t read_helper(void* buf, uint32_t bytes) {
	uint32_t less = buffer_size;
	if (less > bytes)
		less = bytes;
	uint8_t* ubuf = (uint8_t*)buf;
	uint32_t z;
	for (z = 0; z < less; z++) {
		uint8_t c = buffer[circular_inc(&buffer_head, MAX_TERMINAL_BUFFER_LENGTH)];
		buffer_size--;
		ubuf[z] = c;
	}
	return z;
}

// Stores a buffered string of input (blocks until enter is pressed).
// The string returned must support editing of the input string
// using backspace and the arrow keys.
// inputs: file descriptor, buffer pointer and the bytes
// ouutputs: returns how many bytes were read
uint32_t terminal_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	pcb_t* pcb = current_pcb;
	if (termios.lflag & ICANON) {
		while (!(f->mode & FILE_MODE_NONBLOCKING) && !(pcb && pcb->should_terminate)) {
			if (signal_occurring(pcb))
				return -EINTR;
			
			// Check for a terminating character
			bool found = false;
			for (uint32_t z = 0; z < buffer_size; z++) {
				char c = buffer[(buffer_head + z) % MAX_TERMINAL_BUFFER_LENGTH];
				if (c == 0x0 || c == ENTER_KEY || c == EOF_KEY) {
					found = true;
					break;
				}
			}
			if (found)
				break;
			
			schedule();
		}
		
		// Copy bytes one by one, stopping at NULL terminating character or newline
		uint32_t min = buffer_size;
		if (min > bytes)
			min = bytes;
		uint8_t* ubuf = (uint8_t*)buf;
		uint32_t z;
		for (z = 0; z < min; z++) {
			uint8_t c = buffer[circular_inc(&buffer_head, MAX_TERMINAL_BUFFER_LENGTH)];
			buffer_size--;
			if (c == 0x0 || c == EOF_KEY)
				break;
			ubuf[z] = c;
			if (c == ENTER_KEY) {
				z++;
				break;
			}
		}
		
		return z;
	} else {
		uint32_t min = termios.cc[VMIN], time = termios.cc[VTIME];
		if (min == 0 && time == 0) {
			// Return immediately, copy data if available
			return read_helper(buf, bytes);
		} else if (min == 0 && time > 0) {
			// Block until data available or timeout expires (in 1/10th second units)
			struct timeval end = time_add(time_get(), (struct timeval){ time / 10, (time % 10) * US_IN_MS });
			while (buffer_size == 0 && time_less(time_get(), end) &&
				   !(f->mode & FILE_MODE_NONBLOCKING) && !(pcb && pcb->should_terminate)) {
				if (signal_occurring(pcb))
					return -EINTR;
				
				schedule();
			}
			return read_helper(buf, bytes);
		} else if (min > 0 && time == 0) {
			// Block until there are at least min(bytes, MIN) bytes available
			uint32_t less = min;
			if (less > bytes)
				less = bytes;
			while (buffer_size < less &&
				   !(f->mode & FILE_MODE_NONBLOCKING) && !(pcb && pcb->should_terminate)) {
				if (signal_occurring(pcb))
					return -EINTR;
				
				schedule();
			}
			return read_helper(buf, less);
		} else {
			// Block until there is at least one byte available. Then use a timeout for each subsequent byte until
			// the timeout expires or the min(bytes, MIN) have been read
			uint32_t less = min;
			if (less > bytes)
				less = bytes;
			while (buffer_size == 0 &&
				   !(f->mode & FILE_MODE_NONBLOCKING) && !(pcb && pcb->should_terminate)) {
				if (signal_occurring(pcb))
					return -EINTR;
				
				schedule();
			}
			uint32_t orig = buffer_size;
			while (orig < less) {
				struct timeval end = time_add(time_get(), (struct timeval){ time / 10, (time % 10) * US_IN_MS });
				while (buffer_size == orig && time_less(time_get(), end) &&
					   !(f->mode & FILE_MODE_NONBLOCKING) && !(pcb && pcb->should_terminate)) {
					if (signal_occurring(pcb))
						return -EINTR;
					
					schedule();
				}
				if (buffer_size == orig)
					break;
				orig = buffer_size;
			}
			return read_helper(buf, orig);
		}
	}
	
	return 0;
}

// Writes a string to the terminal (returns how many bytes were written)
//inputs: file descriptor, buffer pointer and the bytes
//outputs: -1 for failure and the bytes for success
uint32_t terminal_write(file_descriptor_t* f, const void* buf, uint32_t nbytes) {
	// Loop over and print all the characters
	uint8_t* cbuf = (uint8_t*)buf;
	for(uint32_t i = 0; i < nbytes;) {
		if (!handle_control_chars(cbuf, &i, nbytes))
			putc(cbuf[i++]);
	}
	cursor_position_refresh();
	
	// Return the number of bytes written
	return nbytes;
}

// Get info
uint32_t terminal_stat(file_descriptor_t* f, sys_stat_type* data) {
	data->dev_id = 1;
	data->size = 0;
	data->mode = f->mode;
	return 0;
}

// Seek a terminal (not supported).
uint64_t terminal_llseek(file_descriptor_t* f, uint64_t offset, int whence) {
	return uint64_make(-1, -ESPIPE);
}

// ioctl
#define TCFLUSH			0
#define TCFLOW			1
#define TCGETATTR		2
#define TCSETATTR		3
#define TTYNAME			4
uint32_t terminal_ioctl(file_descriptor_t* f, int request, uint32_t arg1, uint32_t arg2) {
	switch (request) {
		case TCFLUSH: {
			if (arg1 == TCIFLUSH || arg1 == TCIOFLUSH)
				buffer_head = buffer_tail = buffer_size = 0;
			break;
		}
		case TCFLOW: {
			break;
		}
		case TCGETATTR: {
			if (arg1 == (uint32_t)NULL)
				return -EINVAL;
			memcpy((void*)arg1, &termios, sizeof(termios_t));
			break;
		}
		case TCSETATTR: {
			if (arg1 == (uint32_t)NULL)
				return -EINVAL;
			if (arg2 == TCSAFLUSH)
				buffer_head = buffer_tail = buffer_size = 0;
			memcpy(&termios, (void*)arg1, sizeof(termios_t));
			break;
		}
		case TTYNAME: {
			if (arg1 == (uint32_t)NULL)
				return -EINVAL;
			memcpy((void*)arg1, "/dev/tty", 9);
			break;
		}
	}
	
	return 0;
}

// Used for select
bool terminal_can_read(file_descriptor_t* f) {
	if (termios.lflag & ICANON) {
		// Check for a terminating character
		for (uint32_t z = 0; z < buffer_size; z++) {
			char c = buffer[(buffer_head + z) % MAX_TERMINAL_BUFFER_LENGTH];
			if (c == 0x0 || c == ENTER_KEY || c == EOF_KEY) {
				return true;
			}
		}
		return false;
	} else {
		uint32_t min = termios.cc[VMIN];
		return (buffer_size >= min);
	}
}

bool terminal_can_write(file_descriptor_t* f) {
	// Always can write to the terminal
	return true;
}

// Duplicate the file handle
file_descriptor_t* terminal_duplicate(file_descriptor_t* f) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memcpy(d, f, sizeof(file_descriptor_t));
	int namelen = strlen(f->filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, f->filename, namelen + 1);
	d->lock = MUTEX_UNLOCKED;
	
	return d;
}

// Close the terminal
uint32_t terminal_close(file_descriptor_t* fd) {
	kfree(fd->filename);
	return 0;
}
