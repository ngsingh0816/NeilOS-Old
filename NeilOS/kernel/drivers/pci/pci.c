//
//  pci.c
//  NeilOS
//
//  Created by Neil Singh on 12/25/16.
//  Copyright © 2016 Neil Singh. All rights reserved.
//

#include "pci.h"
#include <common/lib.h>

// PCI IO Ports
#define CONFIG_ADDRESS		0xCF8
#define CONFIG_DATA			0xCFC

#define INVALID_DEVICE			0xFFFF

#define NUMBER_OF_BUSES			256
#define NUMBER_OF_DEVICES		32
#define NUMBER_OF_FUNCTIONS		8

#define BAR_TYPE_16				0x01
#define BAR_TYPE_32				0x00
#define BAR_TYPE_64				0x02

#define HEADER_SIZE				0x10

// Read the PCI Configuration Space for a specific bus, device, function, and register.
// The register must be 4byte aligned
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg) {
	uint32_t addr = (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (reg & 0xFC) | 0x80000000;
	
	outl(addr, CONFIG_ADDRESS);
	return inl(CONFIG_DATA);
}

// Write the PCI Configuration Space for a specific bus, device, function, and register.
// The register must be 4byte aligned
void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg, uint32_t val) {
	uint32_t addr = (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (reg & 0xFC) | 0x80000000;
	
	outl(addr, CONFIG_ADDRESS);
	outl(val, CONFIG_DATA);
}

static inline uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t func) {
	return (pci_config_read(bus, device, func, VENDOR_ID_REGISTER) & 0xFFFF);
}

static inline uint16_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t func) {
	return ((pci_config_read(bus, device, func, HEADER_TYPE_REGISTER) >> 16) & 0xFF);
}

// Returns true if this funciton is a match
bool check_function(uint8_t class_code, uint8_t subclass, uint8_t bus, uint8_t device, uint8_t func) {
	uint32_t val = pci_config_read(bus, device, func, CLASS_REGISTER);
	uint16_t cc = (val >> 24) & 0xFF;
	uint16_t sc = (val >> 16) & 0xFF;
	
	/*if (cc != 6 && sc != 4) {
		printf("bus - %d, device - %d, function - %d, class - %d, sub class - %d\n",
		   bus, device, func, cc, sc);
	}*/
	
	return (cc == class_code) && (sc == subclass);
}

// Get info about a specific device
pci_device_t pci_get_info(uint8_t bus, uint8_t device, uint8_t func) {
	pci_device_t header;
	memset(&header, 0, sizeof(header));
	
	// Read the header 4 bytes at a time
	int z;
	for (z = 0; z < HEADER_SIZE; z++)
		header.val[z] = pci_config_read(bus, device, func, z << 2);
	
	header.bus = bus;
	header.device = device;
	header.func = func;
	
	return header;
}

// Set the command value for a device
void pci_set_command(pci_device_t* device, uint16_t cmd) {
	uint32_t val = pci_config_read(device->bus, device->device, device->func, COMMAND_REGISTER);
	
	pci_config_write(device->bus, device->device, device->func, COMMAND_REGISTER, (val & ~(0xFFFF)) | cmd);
	device->command = pci_config_read(device->bus, device->device, device->func, COMMAND_REGISTER) & 0xFFFF;
}

// Retrieve the info for a specific device
pci_device_t pci_device_info(uint8_t class_code, uint8_t subclass) {
	// Do a brute force search
	uint32_t bus, device;
	
	for (bus = 0; bus < NUMBER_OF_BUSES; bus++) {
		for (device = 0; device < NUMBER_OF_DEVICES; device++) {
			// Check if this device is valid
			if (pci_get_vendor_id(bus, device, 0) == INVALID_DEVICE)
				continue;
			
			if (check_function(class_code, subclass, bus, device, 0))
				return pci_get_info(bus, device, 0);
			
			// Check for a multi funciton device
			uint8_t header_type = pci_get_header_type(bus, device, 0);
			if ((header_type >> 7) & 0x1) {
				// Check the remaining functions
				uint32_t func;
				for (func = 1; func < NUMBER_OF_FUNCTIONS; func++) {
					if (pci_get_vendor_id(bus, device, func) == INVALID_DEVICE)
						continue;
					
					if (check_function(class_code, subclass, bus, device, func))
						return pci_get_info(bus, device, func);
				}
			}
		}
	}
	
	// Return an invalid header
	pci_device_t ret;
	ret.bus = -1;
	ret.device = -1;
	return ret;
}

// Get the real address for a bar
uint32_t pci_interpret_bar(uint32_t bar) {
	if (bar & 0x1) {
		// I/O Space BAR
		return bar & ~(0x3);
	}
	
	// Memory Space BAR
	uint8_t type = (bar >> 1) & 0x3;
	if (type == BAR_TYPE_16)
		return bar & 0xFFF0;
	return bar & ~(0xF);
}
