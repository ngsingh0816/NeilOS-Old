//
//  pci.h
//  NeilOS
//
//  Created by Neil Singh on 12/25/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#ifndef PCI_H
#define PCI_H

#include <common/types.h>

#define IDE_CLASS		0x1
#define IDE_SUBCLASS	0x1

#define SATA_CLASS		0x1
#define SATA_SUBCLASS	0x6

#define AUDIO_CLASS		0x4
#define AUDIO_SUBCLASS	0x1

// Header information
#define DEVICE_ID_REGISTER		0x00
#define VENDOR_ID_REGISTER		0x00
#define COMMAND_REGISTER		0x04
#define CLASS_REGISTER			0x08
#define HEADER_TYPE_REGISTER	0x0C
#define IRQ_REGISTER			0x3C

#define PCI_ENABLE_BUSMASTER		0x05

typedef struct {
	union {
		uint32_t val[0x10];
		struct {
			uint16_t vendor_id;
			uint16_t device_id;
			uint16_t command;
			uint16_t status;
			uint8_t revision_id;
			uint8_t prog_if;
			uint8_t subclass;
			uint8_t class_code;
			uint8_t cache_line_size;
			uint8_t latency_timer;
			uint8_t header_type;
			uint8_t bist;
			uint32_t bar[6];
			uint32_t cis_pointer;
			uint16_t subsystem_vendor_id;
			uint16_t subsystem_id;
			uint32_t rom_bar;
			uint8_t capabilites_pointer;
			uint32_t reserved0 : 24;
			uint32_t reserved1;
			uint8_t interrupt_line;
			uint8_t interrupt_pin;
			uint8_t min_grant;
			uint8_t max_latency;
		} __attribute__((packed));
	};
	
	uint8_t bus;
	uint8_t device;
	uint8_t func;
	
} pci_device_t;

// Retrieve the info for a specific device (returns a header with class_code = -1 on failure)
pci_device_t pci_device_info(uint8_t class_code, uint8_t subclass);
// Retrieve the info for a specific device by vendor id
pci_device_t pci_device_info_vendor(uint16_t vendor, uint16_t device);

// Set the command value for a device
void pci_set_command(pci_device_t* device, uint16_t cmd);

// Get the real address for a bar
uint32_t pci_interpret_bar(uint32_t bar);

// Read the PCI Configuration Space for a specific bus, device, function, and register.
// The register must be 4byte aligned
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg);

// Write the PCI Configuration Space for a specific bus, device, function, and register.
// The register must be 4byte aligned
void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val);


#endif 
