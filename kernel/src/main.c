#include <limine.h>
#include <util/cpu.h>
#include <stddef.h>
#include <stdbool.h>
#include <dev/portio.h>
#include <lib/printf.h>
#include <lib/log.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/vma.h>
#include <mm/kmalloc.h>
#include <lib/memory.h>
#include <lib/assert.h>

struct limine_framebuffer *framebuffer = NULL;
uint64_t hhdm_offset = 0;
uint64_t __kernel_phys_base;
uint64_t __kernel_virt_base;
vma_context_t *kernel_vma_context;

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);
__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_executable_address_request kernel_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .response = 0};
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

void kmain(void)
{
    debug("\033c");
    if (LIMINE_BASE_REVISION_SUPPORTED == false)
    {
        error("Unsupported LIMINE base revision, halting");
        hcf();
    }

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
    {
        error("No framebuffer available, halting");
        hcf();
    }

    framebuffer = framebuffer_request.response->framebuffers[0];

    info("shadowOS Kernel v1.0.0");

    BLOCK_START("interrupt_init")
    {
        gdt_init();
        idt_init();
    }
    BLOCK_END("interrupt_init")
    debug_lib_init();

    BLOCK_START("memory_init")
    {
        if (hhdm_request.response == NULL)
        {
            error("No HHDM available, halting");
            hcf();
        }

        hhdm_offset = hhdm_request.response->offset;
        trace("HHDM offset: 0x%.16llx", hhdm_offset);
        pmm_init(memmap_request.response);

        __kernel_phys_base = kernel_address_request.response->physical_base;
        __kernel_virt_base = kernel_address_request.response->virtual_base;

        vmm_init();
        kernel_vma_context = vma_create_context(kernel_pagemap);
    }
    BLOCK_END("memory_init")
   
    size_t ramfs_size = module_request.response->modules[0]->size;
    uint8_t *ramfs_data = (uint8_t *)module_request.response->modules[0]->address;
    (void)ramfs_size;
    (void)ramfs_data;

    info("done");
    hlt();
}
