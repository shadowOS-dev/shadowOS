#include <mm/vmm.h>
#include <mm/pmm.h>
#include <stddef.h>
#include <lib/memory.h>
#include <util/cpu.h>
#include <lib/log.h>

uint64_t *kernel_pagemap;
extern char __limine_requests_start[];
extern char __limine_requests_end[];
extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];
extern uint64_t __kernel_phys_base;
extern uint64_t __kernel_virt_base;

uint64_t virt_to_phys(uint64_t *pagemap, uint64_t virt)
{
    uint64_t pml1_idx = (virt & (uint64_t)0x1ff << 12) >> 12;
    uint64_t pml2_idx = (virt & (uint64_t)0x1ff << 21) >> 21;
    uint64_t pml3_idx = (virt & (uint64_t)0x1ff << 30) >> 30;
    uint64_t pml4_idx = (virt & (uint64_t)0x1ff << 39) >> 39;

    if (!(pagemap[pml4_idx] & 1))
    {
        return 0;
    }

    uint64_t *pml3_table = (uint64_t *)HIGHER_HALF(pagemap[pml4_idx] & 0x000FFFFFFFFFF000);
    if (!(pml3_table[pml3_idx] & 1))
    {
        return 0;
    }

    uint64_t *pml2_table = (uint64_t *)HIGHER_HALF(pml3_table[pml3_idx] & 0x000FFFFFFFFFF000);
    if (!(pml2_table[pml2_idx] & 1))
    {
        return 0;
    }

    uint64_t *pml1_table = (uint64_t *)HIGHER_HALF(pml2_table[pml2_idx] & 0x000FFFFFFFFFF000);
    uint64_t phys_addr = pml1_table[pml1_idx] & 0x000FFFFFFFFFF000;

    return phys_addr;
}

void vmm_map(uint64_t *pagemap, uint64_t virt, uint64_t phys, uint64_t flags)
{
    uint64_t pml1_idx = (virt & (uint64_t)0x1ff << 12) >> 12;
    uint64_t pml2_idx = (virt & (uint64_t)0x1ff << 21) >> 21;
    uint64_t pml3_idx = (virt & (uint64_t)0x1ff << 30) >> 30;
    uint64_t pml4_idx = (virt & (uint64_t)0x1ff << 39) >> 39;

    if (!(pagemap[pml4_idx] & 1))
    {
        pagemap[pml4_idx] = (uint64_t)pmm_request_page() | flags;
    }

    uint64_t *pml3_table = (uint64_t *)HIGHER_HALF(pagemap[pml4_idx] & 0x000FFFFFFFFFF000);
    if (!(pml3_table[pml3_idx] & 1))
    {
        pml3_table[pml3_idx] = (uint64_t)pmm_request_page() | flags;
    }

    uint64_t *pml2_table = (uint64_t *)HIGHER_HALF(pml3_table[pml3_idx] & 0x000FFFFFFFFFF000);
    if (!(pml2_table[pml2_idx] & 1))
    {
        pml2_table[pml2_idx] = (uint64_t)pmm_request_page() | flags;
    }

    uint64_t *pml1_table = (uint64_t *)HIGHER_HALF(pml2_table[pml2_idx] & 0x000FFFFFFFFFF000);
    pml1_table[pml1_idx] = phys | flags;
}

void vmm_unmap(uint64_t *pagemap, uint64_t virt)
{
    uint64_t pml1_idx = (virt & (uint64_t)0x1ff << 12) >> 12;
    uint64_t pml2_idx = (virt & (uint64_t)0x1ff << 21) >> 21;
    uint64_t pml3_idx = (virt & (uint64_t)0x1ff << 30) >> 30;
    uint64_t pml4_idx = (virt & (uint64_t)0x1ff << 39) >> 39;

    if (!(pagemap[pml4_idx] & 1))
    {
        pagemap[pml4_idx] = 0;
        return;
    }

    uint64_t *pml3_table = (uint64_t *)HIGHER_HALF(pagemap[pml4_idx] & 0x000FFFFFFFFFF000);
    if (!(pml3_table[pml3_idx] & 1))
    {
        pml3_table[pml3_idx] = 0;
        return;
    }

    uint64_t *pml2_table = (uint64_t *)HIGHER_HALF(pml3_table[pml3_idx] & 0x000FFFFFFFFFF000);
    if (!(pml2_table[pml2_idx] & 1))
    {
        pml2_table[pml2_idx] = 0;
        return;
    }

    uint64_t *pml1_table = (uint64_t *)HIGHER_HALF(pml2_table[pml2_idx] & 0x000FFFFFFFFFF000);
    pml1_table[pml1_idx] = 0;
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

uint64_t *vmm_new_pagemap()
{
    uint64_t *pagemap = (uint64_t *)HIGHER_HALF(pmm_request_page());
    if (pagemap == NULL)
    {
        error("Failed to allocate page for new pagemap.");
        return NULL;
    }
    memset(pagemap, 0, PAGE_SIZE);
    for (uint64_t i = 256; i < 512; i++)
    {
        pagemap[i] = kernel_pagemap[i];
    }

    debug("Created new pagemap at 0x%.16llx", (uint64_t)pagemap);
    return pagemap;
}

void vmm_destroy_pagemap(uint64_t *pagemap)
{
    trace("Destroying pagemap at 0x%.16llx", (uint64_t)pagemap);
    pmm_release_page((void *)PHYSICAL(pagemap));
    debug("Destroyed pagemap at 0x%.16llx", (uint64_t)pagemap);
}

void vmm_switch_pagemap(uint64_t *new_pagemap)
{

    __asm__ volatile("movq %0, %%cr3" ::"r"(PHYSICAL((uint64_t)new_pagemap)));
}

void vmm_init()
{
    kernel_pagemap = (uint64_t *)HIGHER_HALF(pmm_request_page());
    if (kernel_pagemap == NULL)
    {
        error("Failed to allocate page for kernel pagemap, halting");
        hcf();
    }

    memset(kernel_pagemap, 0, PAGE_SIZE);

    for (uint64_t reqs = ALIGN_DOWN(__limine_requests_start, PAGE_SIZE); reqs < ALIGN_UP(__limine_requests_end, PAGE_SIZE); reqs += PAGE_SIZE)
    {
        vmm_map(kernel_pagemap, reqs, reqs - __kernel_virt_base + __kernel_phys_base, VMM_PRESENT | VMM_WRITE);
    }
    debug("Mapped Limine Requests region.");

    for (uint64_t text = ALIGN_DOWN(__text_start, PAGE_SIZE); text < ALIGN_UP(__text_end, PAGE_SIZE); text += PAGE_SIZE)
    {
        vmm_map(kernel_pagemap, text, text - __kernel_virt_base + __kernel_phys_base, VMM_PRESENT);
    }
    debug("Mapped .text region.");

    for (uint64_t rodata = ALIGN_DOWN(__rodata_start, PAGE_SIZE); rodata < ALIGN_UP(__rodata_end, PAGE_SIZE); rodata += PAGE_SIZE)
    {
        vmm_map(kernel_pagemap, rodata, rodata - __kernel_virt_base + __kernel_phys_base, VMM_PRESENT | VMM_NX);
    }
    debug("Mapped .rodata region.");

    for (uint64_t data = ALIGN_DOWN(__data_start, PAGE_SIZE); data < ALIGN_UP(__data_end, PAGE_SIZE); data += PAGE_SIZE)
    {
        vmm_map(kernel_pagemap, data, data - __kernel_virt_base + __kernel_phys_base, VMM_PRESENT | VMM_WRITE | VMM_NX);
    }
    debug("Mapped .data region.");

    for (uint64_t gb4 = 0; gb4 < 0x100000000; gb4 += PAGE_SIZE)
    {
        vmm_map(kernel_pagemap, (uint64_t)gb4, gb4, VMM_PRESENT | VMM_WRITE);
        vmm_map(kernel_pagemap, (uint64_t)HIGHER_HALF(gb4), gb4, VMM_PRESENT | VMM_WRITE);
    }
    debug("Mapped HHDM.");

    vmm_switch_pagemap(kernel_pagemap);
}