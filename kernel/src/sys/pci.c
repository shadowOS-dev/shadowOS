#include <sys/pci.h>
#include <dev/portio.h>
#include <lib/log.h>

uint32_t pci_read(uint16_t bus, uint16_t device, uint16_t function, uint32_t regoffset)
{
    uint32_t id = 0x1 << 31 | ((bus & 0xFF) << 16) | ((device & 0x1F) << 11) | ((function & 0x07) << 8) | (regoffset & 0xFC);
    outl(0xCF8, id);
    uint32_t result = inl(0xCFC);
    return result >> ((regoffset % 4) << 3);
}

void pci_write(uint16_t bus, uint16_t device, uint16_t function, uint32_t regoffset, uint32_t data)
{
    uint32_t id = 0x1 << 31 | ((bus & 0xFF) << 16) | ((device & 0x1F) << 11) | ((function & 0x07) << 8) | (regoffset & 0xFC);
    outl(0xCF8, id);
    outl(0xCFC, data);
}

pci_device_t get_device_descriptor(uint16_t bus, uint16_t device, uint16_t function)
{
    pci_device_t result;
    result.bus = bus;
    result.device = device;
    result.function = function;

    result.vendor_id = pci_read(bus, device, function, 0x00);
    result.device_id = pci_read(bus, device, function, 0x02);
    result.class_id = pci_read(bus, device, function, 0x0b);
    result.subclass_id = pci_read(bus, device, function, 0x0a);
    result.interface_id = pci_read(bus, device, function, 0x09);
    result.revision = pci_read(bus, device, function, 0x08);
    result.interrupt = pci_read(bus, device, function, 0x3c);

    return result;
}

void pci_debug_log()
{
    for (int bus = 0; bus < 8; bus++)
    {
        for (int device = 0; device < 32; device++)
        {
            for (int function = 0; function < 8; function++)
            {
                pci_device_t desc = get_device_descriptor(bus, device, function);
                if (desc.vendor_id == 0x0000 || desc.vendor_id == 0xFFFF)
                    continue;

                debug("PCI Bus: %d Device: %02d ID: %04X Function: %d Class: %02X Subclass: %02X USB: %s",
                      (uint8_t)(bus & 0xFF),
                      (uint8_t)(device & 0xFF),
                      desc.device_id,
                      (uint8_t)(function & 0xFF),
                      desc.class_id,
                      desc.subclass_id,
                      (desc.class_id == 0x0C && desc.subclass_id == 0x03) ? "yes" : "no");
            }
        }
    }
}
