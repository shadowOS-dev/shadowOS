#ifndef SYS_PCI_H
#define SYS_PCI_H

#include <stdint.h>

typedef struct pci_device
{
    uint32_t base;
    uint32_t interrupt;

    uint16_t bus;
    uint16_t device;
    uint16_t function;

    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t interface_id;

    uint8_t revision;
} pci_device_t;

uint32_t pci_read(uint16_t bus, uint16_t device, uint16_t function, uint32_t regoffset);
void pci_write(uint16_t bus, uint16_t device, uint16_t function, uint32_t regoffset, uint32_t data);
void pci_debug_log();

#endif // SYS_PCI_H