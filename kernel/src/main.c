#include <limine.h>
#include <util/cpu.h>
#include <stddef.h>
#include <stdbool.h>
#include <dev/portio.h>
#include <lib/printf.h>
#include <lib/log.h>
#include <sys/gdt.h>
#include <sys/intr.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/vma.h>
#include <mm/kmalloc.h>
#include <lib/memory.h>
#include <lib/assert.h>
#include <lib/flanterm/backends/fb.h>
#include <lib/flanterm/flanterm.h>
#include <dev/vfs.h>
#include <fs/ramfs.h>
#include <sys/pci.h>
#include <sys/pic.h>
#include <fs/devfs.h>
#include <dev/timer/pit.h>
#include <dev/stdout.h>

struct limine_framebuffer *framebuffer = NULL;
uint64_t hhdm_offset = 0;
uint64_t __kernel_phys_base;
uint64_t __kernel_virt_base;
vma_context_t *kernel_vma_context = NULL;
uint64_t kernel_stack_top = 0;

struct flanterm_context *ft_ctx_priv = NULL;
struct flanterm_context *ft_ctx = NULL;

void (*putchar_impl)(char);

extern char printk_buff[];
extern size_t printk_index;

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

void putchar(char c)
{
    flanterm_write(ft_ctx_priv, &c, 1);
}

void post_main(void);
void kmain(void)
{
    // Save the kernel stack top, given via RSP
    __asm__ volatile("movq %%rsp, %0" : "=r"(kernel_stack_top));

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

    kprintf("shadowOS Kernel v1.0 (c) Copyright 2025 Kevin Alavik <kevin@alavik.se>\n");

    framebuffer = framebuffer_request.response->framebuffers[0];

    ft_ctx_priv = flanterm_fb_init(
        NULL,
        NULL,
        framebuffer->address, framebuffer->width, framebuffer->height, framebuffer->pitch,
        framebuffer->red_mask_size, framebuffer->red_mask_shift,
        framebuffer->green_mask_size, framebuffer->green_mask_shift,
        framebuffer->blue_mask_size, framebuffer->blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0);
    assert(ft_ctx_priv != NULL);
    ft_ctx_priv->cursor_enabled = false;
    ft_ctx_priv->full_refresh(ft_ctx_priv);
    ft_ctx = NULL; // Disable flanterm

    gdt_init();
    idt_init();
    load_idt();

    __asm__ volatile("cli");
    pic_init();
    __asm__ volatile("sti");

    if (hhdm_request.response == NULL)
    {
        error("No HHDM available, halting");
        hcf();
    }

    hhdm_offset = hhdm_request.response->offset;

    pmm_init(memmap_request.response);
    if (kernel_address_request.response == NULL)
    {
        error("No kernel address available, halting");
        hcf();
    }

    __kernel_phys_base = kernel_address_request.response->physical_base;
    __kernel_virt_base = kernel_address_request.response->virtual_base;

    vmm_init();
    // pmm_vmm_cleanup(memmap_request.response);
    kernel_vma_context = vma_create_context(kernel_pagemap);
    if (kernel_vma_context == NULL || kernel_vma_context->root == NULL)
    {
        error("Failed to create kernel VMA context, halting");
        hcf();
    }

    vfs_init();

    msg_assert(module_request.response, "No modules passed to the kernel, expected at least one");

    // unsafe af, but works for now
    uint8_t *ramfs_data = (uint8_t *)module_request.response->modules[0]->address;
    size_t ramfs_size = module_request.response->modules[0]->size;

    // Mount on / and change type to ramfs
    root_mount->type = strdup("ramfs");
    assert(root_mount);
    ramfs_init(root_mount, RAMFS_TYPE_USTAR, ramfs_data, ramfs_size);

    // Setup devfs
    devfs_init();

    // pci shit
    pci_debug_log();

    // Create the /proc directory
    assert(vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/"), "proc", VNODE_DIR));

    // Setup /proc/uptime
    vnode_t *uptime_node = vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/proc"), "uptime", VNODE_FILE);
    assert(uptime_node);
    fprintf(uptime_node, "0.00 0.00");

    // Setup /proc/cpuinfo
    vnode_t *cpuinfo_node = vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/proc"), "cpuinfo", VNODE_FILE);
    assert(cpuinfo_node);

    uint32_t eax, ebx, ecx, edx;
    char vendor[13];
    eax = 0;
    cpuid(eax, &ebx, &ecx, &edx);
    *(uint32_t *)vendor = ebx;
    *(uint32_t *)(vendor + 4) = edx;
    *(uint32_t *)(vendor + 8) = ecx;
    vendor[12] = '\0';

    char cpuinfo[512];
    int len = snprintf(cpuinfo, sizeof(cpuinfo), "vendor_id: %s\n", vendor);
    assert((uint64_t)len < sizeof(cpuinfo));
    fwrite(cpuinfo_node, cpuinfo, len);

    // Ensure /var and /var/log directories exist
    vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/"), "var", VNODE_DIR);
    vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/var"), "log", VNODE_DIR);

    // Ensure /var/log/boot.log file exists
    vfs_create_vnode(vfs_lazy_lookup(VFS_ROOT()->mount, "/var/log"), "boot.log", VNODE_FILE);

    // write the printk buffer to the /var/log/boot.log file
    vnode_t *log = vfs_lazy_lookup(VFS_ROOT()->mount, "/var/log/boot.log");
    assert(log);
    fwrite(log, &printk_buff_start, printk_index);
    vfs_write(log, "\0", 1, log->size);
    memset(&printk_buff_start, 0, printk_index);
    printk_index = 0;

    // clear screen becuz we are done
    ft_ctx_priv->clear(ft_ctx_priv, true);

    // Disable writing directly to the flanterm context, since kprintf will be disabled anyways.
    ft_ctx = NULL;

    // Setup all devices
    stdout_init();
    assert(stdout);

    // init the timer
    pit_init();

    // go to post shit
    info("shadowOS Kernel v1.0 successfully initialized");
    warning("No scheduler available, calling \"post_main\" as an regular function instead >:D");
    post_main();
    hlt();
}
