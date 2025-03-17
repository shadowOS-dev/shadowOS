#include <proc/data/elf.h>
#include <lib/assert.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <config.h>

typedef struct
{
    uint32_t e_magic;
    uint8_t e_class;
    uint8_t e_data;
    uint8_t e_version;
    uint8_t e_osabi;
    uint8_t e_abiversion;
    uint8_t e_pad[7];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version2;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf_header_t;

typedef struct
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf_pheader_t;

typedef struct
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} __attribute__((packed)) elf_sheader_t;

#define ELF_MAGIC 0x464C457F

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7

#define PF_X 0x1 // Execute
#define PF_W 0x2 // Write
#define PF_R 0x4 // Read

uint64_t elf_load_binary(void *data, uint64_t *pagemap)
{
    assert(data);
    elf_header_t *header = (elf_header_t *)data;

    if (header->e_magic != ELF_MAGIC)
    {
        error("Invalid ELF magic: 0x%x", header->e_magic);
        return 0;
    }

    if (header->e_class != 2)
    {
        error("Unsupported ELF class (not 64-bit): %u", header->e_class);
        return 0;
    }

    if (header->e_type != 2)
    {
        error("Unsupported ELF type (not executable): %u", header->e_type);
        return 0;
    }

    elf_pheader_t *ph = (elf_pheader_t *)((uint8_t *)data + header->e_phoff);

    for (uint16_t i = 0; i < header->e_phnum; i++)
    {
        if (ph[i].p_type != PT_LOAD)
            continue;

        uint64_t vaddr_start = ALIGN_DOWN(ph[i].p_vaddr, PAGE_SIZE);
        uint64_t vaddr_end = ALIGN_UP(ph[i].p_vaddr + ph[i].p_memsz, PAGE_SIZE);
        uint64_t offset = ph[i].p_offset;

        uint64_t flags = VMM_PRESENT;
        if (ph[i].p_flags & PF_W)
            flags |= VMM_WRITE;
        if (!(ph[i].p_flags & PF_X))
            flags |= VMM_NX;

        flags |= VMM_USER; // We in usermode baby!

        trace("Loading ELF segment: vaddr 0x%llx - 0x%llx, offset 0x%llx, flags 0x%llx",
              vaddr_start, vaddr_end, offset, flags);

        for (uint64_t vaddr = vaddr_start; vaddr < vaddr_end; vaddr += PAGE_SIZE)
        {
            uint64_t phys = (uint64_t)pmm_request_page();
            if (!phys)
            {
                error("Out of physical memory while loading ELF segment.");
                return 0;
            }

            vmm_map(pagemap, vaddr, phys, flags);
            memset((void *)HIGHER_HALF(phys), 0, PAGE_SIZE);

            uint64_t file_offset = offset + (vaddr - vaddr_start);
            if (file_offset < offset + ph[i].p_filesz)
            {
                uint64_t to_copy = PAGE_SIZE;

                if (file_offset + PAGE_SIZE > offset + ph[i].p_filesz)
                    to_copy = offset + ph[i].p_filesz - file_offset;

                memcpy((void *)HIGHER_HALF(phys), (uint8_t *)data + file_offset, to_copy);

                trace("Copied 0x%llx bytes from ELF file to 0x%llx", to_copy, vaddr);
            }
        }
    }

    trace("ELF loaded successfully, entry point: 0x%llx", header->e_entry);
    return header->e_entry;
}
