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
    .response = 0,
};
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

void kmain(void)
{
    printf("\033c");
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

    gdt_init();
    idt_init();

    if (hhdm_request.response == NULL)
    {
        error("No HHDM available, halting");
        hcf();
    }

    hhdm_offset = hhdm_request.response->offset;
    debug("HHDM offset: 0x%.16llx", hhdm_offset);

    pmm_init(memmap_request.response);
    char *a = HIGHER_HALF(pmm_request_page());
    if (a == NULL)
    {
        error("Failed to allocate page, halting");
        hcf();
    }

    *a = 'A';
    debug("Allocated page at 0x%.16llx, value: %s", (uint64_t)a, a);
    pmm_release_page(PHYSICAL(a));

    if (kernel_address_request.response == NULL)
    {
        error("No kernel address available, halting");
        hcf();
    }

    __kernel_phys_base = kernel_address_request.response->physical_base;
    __kernel_virt_base = kernel_address_request.response->virtual_base;

    vmm_init();
    kernel_vma_context = vma_create_context(kernel_pagemap);
    if (kernel_vma_context == NULL)
    {
        error("Failed to create kernel VMA context, halting");
        hcf();
    }

    char *b = (char *)vma_alloc(kernel_vma_context, 1, VMM_PRESENT | VMM_WRITE);
    if (b == NULL)
    {
        error("Failed to allocate virtual page, halting");
        hcf();
    }

    *b = 'B';
    debug("Allocated virtual page at 0x%.16llx, value: %s", (uint64_t)b, b);
    vma_free(kernel_vma_context, b);

    for (int i = 0; i < 10; i++)
    {
        char *c = (char *)kmalloc(sizeof(char));
        if (c == NULL)
        {
            error("Failed to allocate virtual page on kernel heap, halting");
            hcf();
        }
        *c = 'C';
        debug("Allocated virtual page on the kernel heap at 0x%.16llx, value: %s", (uint64_t)c, c);
        kfree(c);
    }

    info("uwu :3");

    hlt();
}
