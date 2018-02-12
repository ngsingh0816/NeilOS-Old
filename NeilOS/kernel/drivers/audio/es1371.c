//
//  es1371.c
//  NeilOS
//
//  Created by Neil Singh on 1/2/18.
//  Copyright © 2018 Neil Singh. All rights reserved.
//

#include "es1371.h"
#include <drivers/pci/pci.h>
#include <memory/memory.h>
#include <drivers/pic/i8259.h>
#include <syscalls/interrupt.h>

#define CONTROL					0x00
#define STATUS					0x04
#define RESET_ADDR				0x07
#define UART_DATA				0x08
#define UART_STATUS				0x09
#define UART_TEST				0x0A
#define MEMORY_PAGE				0x0C
#define SAMPLE_RATE_CONVERTER	0x10
#define CODEC_READ_WRITE		0x14
#define LEGACY					0x18
#define SERIAL_INTERFACE		0x20
#define PLAYBACK1_FRAME_COUNT	0x24
#define PLAYBACK2_FRAME_COUNT	0x28
#define RECORD_FRAME_COUNT		0x2C
#define PLAYBACK1_BUFFER_ADDR	0x30
#define PLAYBACK1_BUFFER_DEF	0x34
#define PLAYBACK2_BUFFER_ADDR	0x38
#define PLAYBACK2_BUFFER_DEF	0x3C
#define RECORD_BUFFER_ADDR		0x30
#define RECORD_BUFFER_DEF		0x34
#define UART_FIFO_DATA			0x30

#define RESET_COMMAND			0x20
#define ENABLE_REGS				0xFF

#define SRC_WE					(1 << 24)
#define SRC_DISABLE				(1 << 22)
#define SRC_BUSY				(1 << 23)

#define CODEC_WD				(1 << 23)
#define CODEC_RDY				(1 << 31)

#define SRC_FREQ_LOW			0x75
#define SRC_FREQ_HIGH			0x77
#define SAMPLING_RATE			44100
// Note: this number must be divisible by 2
#define NUM_SAMPLES				(SAMPLING_RATE/10)
#define NUM_SECTIONS			2
#define SECTION_SIZE			(NUM_SAMPLES * 2 / NUM_SECTIONS)

#define SI_ENABLE_INT			0x0020020C
#define SI_DISABLE_INT			0x0020000C
#define SI_INT					0x00000200

#define WTSRSEL_SH				12
#define PCLKDIV_SH				16

#define CONTROL_PLAY			0x20

// AC97 Regs
#define MASTER_VOLUME			0x2
#define PCM_VOLUME				0x18

#define ES1370_ID				0x50001274				// Qemu
#define ES1371_ID				0x13711274				// VMWare

typedef enum {
	ES1370,
	ES1371,
} es_dev_type;

int16_t sound_data[NUM_SAMPLES * 2] __attribute__((aligned(64 * 1024)));
// Delayed buffering
volatile uint32_t sound_buffer_id = 0;
uint32_t buffer_index = NUM_SECTIONS - 1;
uint32_t num_devices_open = 0;
mutex_t devices_open_lock = MUTEX_UNLOCKED;
spinlock_t sound_lock = SPIN_LOCK_UNLOCKED;
bool sound_first = true;

uint32_t es_base_port = 0;
es_dev_type es_type = ES1370;

float master_volume = 1.0;

typedef struct {
	float volume;
} es_info;

void src_set_enabled(bool enabled) {
	uint32_t reg = 0;
	if (!enabled)
		reg = SRC_DISABLE;
	outl(reg, es_base_port + SAMPLE_RATE_CONVERTER);
}

void src_write(uint32_t addr, uint16_t data) {
	uint32_t reg = (addr << 25) | SRC_WE | data;
	outl(reg, es_base_port + SAMPLE_RATE_CONVERTER);
	while (inl(es_base_port + SAMPLE_RATE_CONVERTER) & SRC_BUSY) {}
}

uint16_t src_read(uint32_t addr) {
	uint32_t reg = (addr << 25);
	outl(reg, es_base_port + SAMPLE_RATE_CONVERTER);
	uint32_t val = 0;
	while ((val = inl(es_base_port + SAMPLE_RATE_CONVERTER)) & SRC_BUSY) {}
	return (val & 0xFFFF);
}

void codec_write(uint32_t addr, uint16_t data) {
	uint32_t reg = (addr << 16) | data;
	outl(reg, es_base_port + CODEC_READ_WRITE);
}

uint16_t codec_read(uint32_t addr) {
	uint32_t reg = (addr << 16) | CODEC_WD;
	outl(reg, es_base_port + CODEC_READ_WRITE);
	uint32_t val = 0;
	while (!((val = inl(es_base_port + CODEC_READ_WRITE)) & CODEC_RDY)) {}
	return (val & 0xFFFF);
}

void set_sample_rate(uint32_t rate) {
	if (es_type == ES1371) {
		// Set the sample rate converter
		src_set_enabled(false);
		uint32_t freq = ((SAMPLING_RATE << 15) + 1500) / 3000;
		src_write(SRC_FREQ_LOW, (freq >> 5) & 0xFC00);
		src_write(SRC_FREQ_HIGH, freq & 0x7FFF);
		src_set_enabled(true);
	} else {
		static uint32_t sample_ratesDAC1[4] = { 5512, 11025, 22050, 44100 };
		int index = 3;
		for (int z = 0; z < 4; z++) {
			if (sample_ratesDAC1[z] == z) {
				index = z;
				break;
			}
		}
		uint32_t val = inl(es_base_port + CONTROL);
		val |= (index & 0x3) << WTSRSEL_SH;
		uint32_t div = (((1411200+(rate)/2)/(rate))-2);
		val |= (div & ((1 << 13)-1)) << PCLKDIV_SH;
		outl(val, es_base_port + CONTROL);
	}
}

void es_handler(int irq) {
	if (!((inl(es_base_port + STATUS) >> 1) & 0x1)) {
		send_eoi(irq);
		return;
	}
	
	// Disable this interrupt
	disable_irq(irq);
	
	uint32_t val = inl(es_base_port + SERIAL_INTERFACE);
	outl(val & ~SI_INT, es_base_port + SERIAL_INTERFACE);
	outl(val | SI_INT, es_base_port + SERIAL_INTERFACE);
	
	// Make writes happen on the buffer that just played
	uint32_t samples_left = inl(es_base_port + PLAYBACK2_BUFFER_DEF) >> 16;
	buffer_index = ((NUM_SECTIONS * samples_left) / (NUM_SAMPLES - 1)) - 1;
	if (buffer_index == -1)
		buffer_index = NUM_SECTIONS - 1;
	// Have the next write overwrite everything, rather than mix
	sound_first = true;
	
	// Let readers know we are ready for the next batch of data
	sound_buffer_id++;
	
	// Say the we've handled this interrupt
	send_eoi(irq);
	enable_irq(irq);
}

void es_init() {
	memset(sound_data, 0, NUM_SAMPLES * 2 * sizeof(int16_t));
	
	// Enable busmastering
	pci_device_t dev = pci_device_info(AUDIO_CLASS, AUDIO_SUBCLASS);
	es_base_port = pci_interpret_bar(dev.bar[0]);
	pci_set_command(&dev, PCI_ENABLE_BUSMASTER);
	uint32_t dev_id = pci_config_read(dev.bus, dev.device, dev.func, VENDOR_ID_REGISTER);
	uint32_t irq_num = pci_config_read(dev.bus, dev.device, dev.func, IRQ_REGISTER) & 0xFF;
	if (dev_id == ES1370_ID) {
		es_type = ES1370;
		printf("Initializing ES1370 (irq: 0x%x)\n", irq_num);
	} else if (dev_id == ES1371_ID) {
		es_type = ES1371;
		printf("Initializing ES1371 (irq: 0x%x)\n", irq_num);
	} else {
		printf("Invalid ES1370/1 Device\n");
		return;
	}
	
	if (es_type == ES1371) {
		// Set volume max
		codec_write(MASTER_VOLUME, 0);
		codec_write(PCM_VOLUME, 0x808);
	}
	set_sample_rate(SAMPLING_RATE);
	
	// Memory page
	outb(0x0C, es_base_port + MEMORY_PAGE);
	
	request_irq(irq_num, es_handler);
	enable_irq(irq_num);
}

void increment_dev_open() {
	down(&devices_open_lock);
	num_devices_open++;
	if (num_devices_open == 1) {
		memset(sound_data, 0, sizeof(int16_t) * NUM_SAMPLES * 2);
		
		// Start playing things
		outl((uint32_t)sound_data - VM_KERNEL_ADDRESS, es_base_port + PLAYBACK2_BUFFER_ADDR);
		// Buffer size in dwords - 1
		outl(NUM_SAMPLES - 1, es_base_port + PLAYBACK2_BUFFER_DEF);
		// Number of samples per interrupt - 1
		outl(NUM_SAMPLES / NUM_SECTIONS - 1, es_base_port + PLAYBACK2_FRAME_COUNT);
		
		outl(SI_ENABLE_INT, es_base_port + SERIAL_INTERFACE);
		outl(inl(es_base_port + CONTROL) & ~(CONTROL_PLAY), es_base_port + CONTROL);
		outl(inl(es_base_port + CONTROL) | CONTROL_PLAY, es_base_port + CONTROL);
	}
	up(&devices_open_lock);
}

void decrement_dev_open() {
	down(&devices_open_lock);
	num_devices_open--;
	if (num_devices_open == 0) {
		// Stop playing things
		outl(inl(es_base_port + CONTROL) & ~CONTROL_PLAY, es_base_port + CONTROL);
	}
	up(&devices_open_lock);
}

// Open the sound device
file_descriptor_t* es_open(const char* filename, uint32_t mode) {
	file_descriptor_t* d = (file_descriptor_t*)kmalloc(sizeof(file_descriptor_t));
	if (!d)
		return NULL;
	memset(d, 0, sizeof(file_descriptor_t));
	// Mark in use
	d->lock = MUTEX_UNLOCKED;
	d->type = AUDIO_FILE_TYPE;
	d->mode = FILE_TYPE_CHARACTER | mode;
	int namelen = strlen(filename);
	d->filename = kmalloc(namelen + 1);
	if (!d->filename) {
		kfree(d);
		return NULL;
	}
	memcpy(d->filename, filename, namelen + 1);
	es_info* info = kmalloc(sizeof(es_info));
	d->info = info;
	if (!d->info) {
		kfree(d->filename);
		kfree(d);
		return NULL;
	}
	info->volume = 1.0f;
	
	// Assign the functions
	d->read = es_read;
	d->write = es_write;
	d->stat = es_stat;
	d->llseek = es_llseek;
	d->ioctl = es_ioctl;
	d->duplicate = es_duplicate;
	d->close = es_close;
	
	increment_dev_open();
	
	return d;
}

// Blocks until more data is needed then returns the amount of bytes needed to write
uint32_t es_read(file_descriptor_t* f, void* buf, uint32_t bytes) {
	int id = sound_buffer_id;
	pcb_t* pcb = current_pcb;
	while (id == sound_buffer_id && !(pcb && pcb->should_terminate)) {
		if (signal_occurring(pcb))
			return -EINTR;
		schedule();
	}
	
	return SECTION_SIZE * sizeof(int16_t);
}

// Write sound data
uint32_t es_write(file_descriptor_t* f, const void* buf, uint32_t nbytes) {
	if (nbytes != SECTION_SIZE * sizeof(int16_t))
		return -EINVAL;
	
	es_info* info = f->info;
	
	// Simple addition mixing
	spin_lock(&sound_lock);
	int bi = buffer_index;
	if (sound_first) {
		int16_t* ibuf = (int16_t*)buf;
		int16_t* isb = &sound_data[bi * SECTION_SIZE];
		for (int z = 0; z < SECTION_SIZE; z++)
			isb[z] = (int16_t)((float)ibuf[z] * info->volume * master_volume);
	}
	else {
		int16_t* ibuf = (int16_t*)buf;
		int16_t* isb = &sound_data[bi * SECTION_SIZE];
		for (int z = 0; z < SECTION_SIZE; z++)
			isb[z] += (int16_t)((float)ibuf[z] * info->volume * master_volume);

	}
	spin_unlock(&sound_lock);
	
	return SECTION_SIZE * sizeof(int16_t);
}

// Get info
uint32_t es_stat(file_descriptor_t* f, sys_stat_type* data) {
	data->dev_id = f->type;
	data->size = 0;
	data->mode = f->mode;
	return 0;
}

// Seek (returns error)
uint64_t es_llseek(file_descriptor_t* f, uint64_t offset, int whence) {
	return uint64_make(-1, -ESPIPE);
}

// set_volume(float volume [0:1])
#define ES_IOCTL_SET_VOLUME		0
// get_volume(float* volume)
#define ES_IOCTL_GET_VOLUME		1
// set_master_volume(float volume [0:1])
#define ES_IOCTL_SET_MASTER_VOLUME	2
// get_master_volume(float* volume)
#define ES_IOCTL_GET_MASTER_VOLUME	3

// ioctl
uint32_t es_ioctl(file_descriptor_t* f, int request, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
	es_info* info = f->info;
	switch (request) {
		case ES_IOCTL_SET_VOLUME: {
			info->volume = *((float*)&arg1);
			break;
		}
		case ES_IOCTL_GET_VOLUME: {
			if (!arg1)
				return -EINVAL;
			*((float*)arg1) = info->volume;
			break;
		}
		case ES_IOCTL_SET_MASTER_VOLUME: {
			master_volume = *((float*)&arg1);
			break;
		}
		case ES_IOCTL_GET_MASTER_VOLUME: {
			if (!arg1)
				return -EINVAL;
			*((float*)arg1) = master_volume;
			break;
		}
	}
	
	return -EINVAL;
}

// Duplicate the file handle
file_descriptor_t* es_duplicate(file_descriptor_t* f) {
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
	d->info = kmalloc(sizeof(es_info));
	if (!d->info) {
		kfree(d->filename);
		kfree(d);
		return NULL;
	}
	memcpy(d->info, f->info, sizeof(es_info));
	d->lock = MUTEX_UNLOCKED;
	
	increment_dev_open();
	
	return d;
}

// Close the sound device
uint32_t es_close(file_descriptor_t* fd) {
	kfree(fd->info);
	kfree(fd->filename);
	decrement_dev_open();
	return 0;
}
